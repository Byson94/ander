#include <jni.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#define SOCKET_PATH  "/tmp/ander-launcher.sock"
#define MSG_CALL 0x01
#define MSG_RETURN 0x02
#define MSG_JNIENV 0x03
#define MSG_JNIENV_RETURN 0x04
#define MSG_JNIENV_RETURN_DATA 0x05
#define MSG_ERROR 0x06
#define BUF_SIZE 65536

typedef struct {
    uint8_t  type;
    uint32_t id;
    uint32_t data_len;
} __attribute__((packed)) MsgHeader;

static int      g_sock    = -1;
static uint32_t g_call_id = 0;
static JavaVM  *g_jvm     = NULL;

// === IO helpers ===                                 
static int write_all(int fd, const void *buf, size_t len) {
    const uint8_t *p = buf;
    while (len > 0) {
        ssize_t n = write(fd, p, len);
        if (n <= 0) return -1;
        p += n; len -= n;
    }
    return 0;
}

static int read_all(int fd, void *buf, size_t len) {
    uint8_t *p = buf;
    while (len > 0) {
        ssize_t n = read(fd, p, len);
        if (n <= 0) return -1;
        p += n; len -= n;
    }
    return 0;
}

static int send_msg(uint8_t type, uint32_t id, const void *data, uint32_t len) {
    MsgHeader hdr = { type, id, len };
    if (write_all(g_sock, &hdr, sizeof(hdr)) < 0) return -1;
    if (len > 0 && write_all(g_sock, data, len) < 0) return -1;
    return 0;
}

static int recv_msg(MsgHeader *hdr, void *buf, uint32_t buf_size) {
    if (read_all(g_sock, hdr, sizeof(MsgHeader)) < 0) return -1;
    if (hdr->data_len > buf_size) return -1;
    if (hdr->data_len > 0 && read_all(g_sock, buf, hdr->data_len) < 0) return -1;
    return 0;
}

// === JNIEnv callback handler ===
static int64_t handle_jnienv_scalar(JNIEnv *env, const char *method,
                                     uint8_t *args, uint32_t args_len) {
    if (strcmp(method, "FindClass") == 0) {
        char *name = (char *)args;
        jclass cls = (*env)->FindClass(env, name);
        return (int64_t)(intptr_t)cls;
    }
    if (strcmp(method, "GetMethodID") == 0) {
        jobject obj;
        memcpy(&obj, args, sizeof(obj));
        char *name = (char *)(args + sizeof(obj));
        char *sig  = name + strlen(name) + 1;
        jclass cls = (*env)->GetObjectClass(env, obj);
        jmethodID mid = (*env)->GetMethodID(env, cls, name, sig);
        return (int64_t)(intptr_t)mid;
    }
    if (strcmp(method, "GetObjectClass") == 0) {
        jobject obj;
        memcpy(&obj, args, sizeof(obj));
        jclass cls = (*env)->GetObjectClass(env, obj);
        return (int64_t)(intptr_t)cls;
    }
    if (strcmp(method, "NewGlobalRef") == 0) {
        jobject obj;
        memcpy(&obj, args, sizeof(obj));
        jobject ref = (*env)->NewGlobalRef(env, obj);
        return (int64_t)(intptr_t)ref;
    }
    if (strcmp(method, "GetArrayLength") == 0) {
        jarray arr;
        memcpy(&arr, args, sizeof(arr));
        return (int64_t)(*env)->GetArrayLength(env, arr);
    }
    if (strcmp(method, "GetFloatArrayElements") == 0) {
        jfloatArray arr;
        memcpy(&arr, args, sizeof(arr));
        jfloat *elems = (*env)->GetFloatArrayElements(env, arr, NULL);
        return (int64_t)(intptr_t)elems;
    }
    if (strcmp(method, "ReleaseFloatArrayElements") == 0) {
        jfloatArray arr; jfloat *elems; jint mode;
        memcpy(&arr,   args,                              sizeof(arr));
        memcpy(&elems, args + sizeof(arr),                sizeof(elems));
        memcpy(&mode,  args + sizeof(arr)+sizeof(elems),  sizeof(mode));
        (*env)->ReleaseFloatArrayElements(env, arr, elems, mode);
        return 0;
    }
    if (strcmp(method, "NewFloatArray") == 0) {
        jsize len; memcpy(&len, args, sizeof(len));
        jfloatArray arr = (*env)->NewFloatArray(env, len);
        return (int64_t)(intptr_t)arr;
    }
    if (strcmp(method, "SetFloatArrayRegion") == 0) {
        jfloatArray arr; jsize start, len;
        memcpy(&arr,   args, sizeof(arr));
        memcpy(&start, args+sizeof(arr), sizeof(start));
        memcpy(&len,   args+sizeof(arr)+sizeof(start), sizeof(len));
        jfloat *buf = (jfloat *)(args+sizeof(arr)+sizeof(start)+sizeof(len));
        (*env)->SetFloatArrayRegion(env, arr, start, len, buf);
        return 0;
    }
    if (strcmp(method, "ExceptionCheck") == 0) {
        return (int64_t)(*env)->ExceptionCheck(env);
    }
    if (strcmp(method, "ExceptionClear") == 0) {
        (*env)->ExceptionClear(env);
        return 0;
    }
    if (strcmp(method, "ReleasePrimitiveArrayCritical") == 0) {
        // Data was already copied on GetPrimitiveArrayCritical
        return 0;
    }
    fprintf(stderr, "[AnderBridge] unimplemented JNIENV: %s\n", method);
    return 0;
}

// Sends response directly. This handles both scalar and data responses.
static void handle_jnienv_and_respond(JNIEnv *env, uint8_t *data,
                                       uint32_t dlen, uint32_t call_id) {
    char *method   = (char *)data;
    uint8_t *args  = data + strlen(method) + 1;
    uint32_t args_len = dlen - (strlen(method) + 1);

    fprintf(stderr, "[AnderBridge] JNIENV: %s\n", method);

    fprintf(stderr, "[AnderBridge] method bytes: ");
    for (int i = 0; i < 30 && i < (int)dlen; i++)
        fprintf(stderr, "%02x ", data[i]);
    fprintf(stderr, "\n");
    fprintf(stderr, "[AnderBridge] method len=%zu dlen=%u args_len=%u\n",
            strlen(method), dlen, args_len);
    fflush(stderr);

    if (strcmp(method, "GetPrimitiveArrayCritical") == 0) {
        jarray arr;
        memcpy(&arr, args, sizeof(arr));
        fprintf(stderr, "[AnderBridge] GetPrimitiveArrayCritical arr=%p\n", (void*)arr);
        fflush(stderr);

        jsize len = (*env)->GetArrayLength(env, arr);
        fprintf(stderr, "[AnderBridge] array len=%d\n", (int)len);
        fflush(stderr);
        void *ptr = (*env)->GetPrimitiveArrayCritical(env, arr, NULL);
        fprintf(stderr, "[AnderBridge] ptr=%p\n", ptr);
        fflush(stderr);
        if (!ptr || len == 0) {
            int64_t zero = 0;
            send_msg(MSG_JNIENV_RETURN, call_id, &zero, sizeof(zero));
            return;
        }

        // payload: handle(8 bytes) + raw array data
        uint32_t total = sizeof(int64_t) + len;
        uint8_t *resp  = malloc(total);
        int64_t handle = (int64_t)(intptr_t)arr;
        memcpy(resp, &handle, sizeof(handle));
        memcpy(resp + sizeof(handle), ptr, len);
        (*env)->ReleasePrimitiveArrayCritical(env, arr, ptr, JNI_ABORT);

        send_msg(MSG_JNIENV_RETURN_DATA, call_id, resp, total);
        free(resp);
        return;
    }
    if (strcmp(method, "NewDirectByteBuffer") == 0) {
        void *arm_addr; jlong capacity;
        memcpy(&arm_addr, args,              sizeof(arm_addr));
        memcpy(&capacity, args+sizeof(arm_addr), sizeof(capacity));
        
        uint8_t *pixel_data = args + sizeof(arm_addr) + sizeof(capacity);
        
        // Allocate x86_64 buffer and copy pixel data into it
        void *host_buf = malloc(capacity);
        memcpy(host_buf, pixel_data, capacity);
        
        jobject direct_buf = (*env)->NewDirectByteBuffer(env, host_buf, capacity);
        // NOTE: host_buf intentionally not freed. Java ByteBuffer owns it.
        
        int64_t result = (int64_t)(intptr_t)direct_buf;
        send_msg(MSG_JNIENV_RETURN, call_id, &result, sizeof(result));
        return;
    }
    if (strcmp(method, "NewStringUTF") == 0) {
        char *str = (char *)args;
        jstring s = (*env)->NewStringUTF(env, str);
        int64_t result = (int64_t)(intptr_t)s;
        send_msg(MSG_JNIENV_RETURN, call_id, &result, sizeof(result));
        return;
    }

    int64_t result = handle_jnienv_scalar(env, method, args, args_len);
    send_msg(MSG_JNIENV_RETURN, call_id, &result, sizeof(result));
}

// === Generic dispatcher ===
// This is registered as the implementation for ALL native methods.
// It figures out the symbol name from the class+method, sends MSG_CALL,
// handles any MSG_JNIENV callbacks, and returns the result.
static jlong generic_dispatcher(JNIEnv *env, jobject obj, ...) {
    // We can't know the symbol name here without extra context.
    // So we store it in thread-local state set by the trampoline.
    // For now this is a placeholder. The trampolines set g_current_symbol.
    fprintf(stderr, "[AnderBridge] generic_dispatcher called (no symbol context)\n");
    return 0;
}

// === Symbol-aware trampoline ===
// Each native method gets its own trampoline that knows its symbol name.
// We generate these dynamically at registration time.

typedef struct {
    char     symbol[256];
    JNIEnv  *env;
} TrampolineCtx;

static jlong trampoline_call(JNIEnv *env, jobject obj,
                              const char *symbol,
                              uint64_t *args, uint32_t argc) {
    fprintf(stderr, "[AnderBridge] trampoline_call: %s g_sock=%d\n", symbol, g_sock);
    fflush(stderr);
    if (g_sock < 0) {
        fprintf(stderr, "[AnderBridge] NOT CONNECTED!\n");
        fflush(stderr);
        return 0;
    }
                        
    // Build payload: symbol\0 + args
    uint32_t sym_len  = strlen(symbol) + 1;
    uint32_t args_len = argc * 8;
    uint32_t total    = sym_len + args_len;
    uint8_t *payload  = malloc(total);
    memcpy(payload, symbol, sym_len);
    if (args_len > 0) memcpy(payload + sym_len, args, args_len);

    uint32_t id = ++g_call_id;
    send_msg(MSG_CALL, id, payload, total);
    free(payload);

    // Handle responses
    MsgHeader hdr;
    uint8_t  *buf = malloc(BUF_SIZE);
    jlong result = 0;

    while (1) {
        if (recv_msg(&hdr, buf, BUF_SIZE) < 0) break;

        if (hdr.type == MSG_RETURN && hdr.id == id) {
            memcpy(&result, buf, sizeof(result));
            break;
        } else if (hdr.type == MSG_JNIENV && hdr.id == id) {
            handle_jnienv_and_respond(env, buf, hdr.data_len, id);
        } else if (hdr.type == MSG_ERROR) {
            fprintf(stderr, "[AnderBridge] error: %s\n", (char *)buf);
            break;
        }
    }

    free(buf);
    return result;
}

// === Native method registration ===
// Scans all classes in the APK jar for native methods and registers
// them with trampolines that route over the socket.

// We need one trampoline function per native method since each needs
// to know its own symbol name. We use a fixed-size pool.
#define MAX_NATIVES 1024

typedef struct {
    char    symbol[256];
    int     param_count;
} NativeSlot;

static NativeSlot g_natives[MAX_NATIVES];
static int        g_native_count = 0;

// Trampoline functions — one per slot
// Each reads its symbol from g_natives[N].symbol
#define MAKE_TRAMPOLINE(N) \
static jlong trampoline_##N(JNIEnv *env, jobject obj, \
    uint64_t a0, uint64_t a1, uint64_t a2, uint64_t a3, \
    uint64_t a4, uint64_t a5, uint64_t a6, uint64_t a7) { \
    uint64_t args[] = {a0,a1,a2,a3,a4,a5,a6,a7}; \
    return trampoline_call(env, obj, g_natives[N].symbol, \
                           args, g_natives[N].param_count); \
}

#include "trampolines_generated.h"

// Derive JNI symbol name from class + method
// e.g. com.badlogic.gdx.graphics.g2d.Gdx2DPixmap + load
//   -> Java_com_badlogic_gdx_graphics_g2d_Gdx2DPixmap_load
static void derive_symbol(const char *class_name, const char *method_name,
                           char *out, size_t out_size) {
    snprintf(out, out_size, "Java_");
    size_t off = 5;
    for (const char *p = class_name; *p && off < out_size-1; p++, off++) {
        out[off] = (*p == '.') ? '_' : *p;
    }
    if (off < out_size-1) { out[off++] = '_'; }
    for (const char *p = method_name; *p && off < out_size-1; p++, off++) {
        out[off] = *p;
    }
    out[off] = '\0';
}

// === Java API ===                              
JNIEXPORT void JNICALL Java_AnderBridge_registerNativeMethod(
        JNIEnv *env, jclass cls,
        jstring jclass_name, jstring jmethod_name, jstring jsig) {

    const char *class_name  = (*env)->GetStringUTFChars(env, jclass_name, NULL);
    const char *method_name = (*env)->GetStringUTFChars(env, jmethod_name, NULL);
    const char *sig         = (*env)->GetStringUTFChars(env, jsig, NULL);

    char slash_name[256];
    strncpy(slash_name, class_name, 255);
    for (char *p = slash_name; *p; p++)
        if (*p == '.') *p = '/';

    jclass target = (*env)->FindClass(env, slash_name);
    if (!target) {
        fprintf(stderr, "[AnderBridge] class not found: %s\n", class_name);
        (*env)->ExceptionClear(env);
        goto cleanup;
    }

    if (g_native_count >= (int)MAX_TRAMPOLINES) {
        fprintf(stderr, "[AnderBridge] too many native methods!\n");
        goto cleanup;
    }

    // Derive symbol name
    char symbol[256];
    derive_symbol(class_name, method_name, symbol, sizeof(symbol));

    // Count params from signature
    int param_count = 0;
    const char *p = sig + 1;
    while (*p && *p != ')') {
        if (*p == 'L') { while (*p && *p != ';') p++; }
        else if (*p != '[') param_count++;
        if (*p) p++;
    }

    // Store in slot
    int slot = g_native_count++;
    strncpy(g_natives[slot].symbol, symbol, 255);
    g_natives[slot].param_count = param_count;

    // Register with correct signature from Java
    JNINativeMethod nm;
    nm.name      = (char *)method_name;
    nm.signature = (char *)sig;
    nm.fnPtr     = g_trampolines[slot];

    int ret = (*env)->RegisterNatives(env, target, &nm, 1);
    if (ret != 0) {
        fprintf(stderr, "[AnderBridge] RegisterNatives failed: %s.%s %s\n",
                class_name, method_name, sig);
        (*env)->ExceptionClear(env);
        g_native_count--;
    } else {
        fprintf(stderr, "[AnderBridge] registered: %s.%s%s -> slot %d\n",
                class_name, method_name, sig, slot);
    }

cleanup:
    (*env)->ReleaseStringUTFChars(env, jclass_name, class_name);
    (*env)->ReleaseStringUTFChars(env, jmethod_name, method_name);
    (*env)->ReleaseStringUTFChars(env, jsig, sig);
}

JNIEXPORT void JNICALL Java_AnderBridge_connect(JNIEnv *env, jclass cls) {
    struct sockaddr_un addr = {0};
    g_sock = socket(AF_UNIX, SOCK_STREAM, 0);
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, SOCKET_PATH, sizeof(addr.sun_path)-1);
    if (connect(g_sock, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("[AnderBridge] connect failed");
        g_sock = -1;
    } else {
        fprintf(stderr, "[AnderBridge] connected to %s\n", SOCKET_PATH);
    }
}

JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM *vm, void *reserved) {
    g_jvm = vm;
    fprintf(stderr, "[AnderBridge] JNI_OnLoad\n");
    return JNI_VERSION_1_6;
}