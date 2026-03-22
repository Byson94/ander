#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <jni.h>
#include "ipc.h"
#include "trampolines.h"

static JavaVM *g_jvm = NULL;

static void derive_symbol(const char *class_name, const char *method_name,
                           char *out, size_t out_size) {
    snprintf(out, out_size, "Java_");
    size_t off = 5;
    for (const char *p = class_name; *p && off < out_size-1; p++, off++)
        out[off] = (*p == '.') ? '_' : *p;
    if (off < out_size-1) { out[off++] = '_'; }
    for (const char *p = method_name; *p && off < out_size-1; p++, off++)
        out[off] = *p;
    out[off] = '\0';
}

JNIEXPORT void JNICALL Java_AnderBridge_registerNativeMethod(
        JNIEnv *env, jclass cls,
        jstring jclass_name, jstring jmethod_name, jstring jsig) {

    const char *class_name  = (*env)->GetStringUTFChars(env, jclass_name,  NULL);
    const char *method_name = (*env)->GetStringUTFChars(env, jmethod_name, NULL);
    const char *sig         = (*env)->GetStringUTFChars(env, jsig,         NULL);

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

    char symbol[256];
    derive_symbol(class_name, method_name, symbol, sizeof(symbol));

    int param_count = 0;
    const char *p = sig + 1;
    while (*p && *p != ')') {
        if (*p == 'L') { while (*p && *p != ';') p++; }
        else if (*p != '[') param_count++;
        if (*p) p++;
    }

    int slot = g_native_count++;
    strncpy(g_natives[slot].symbol, symbol, 255);
    g_natives[slot].param_count = param_count;

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
    (*env)->ReleaseStringUTFChars(env, jclass_name,  class_name);
    (*env)->ReleaseStringUTFChars(env, jmethod_name, method_name);
    (*env)->ReleaseStringUTFChars(env, jsig,         sig);
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

JNIEXPORT jobjectArray JNICALL Java_AnderBridge_listNativeSymbols(JNIEnv *env, jclass cls) {
    if (g_sock < 0) {
        fprintf(stderr, "[AnderBridge] listNativeSymbols: not connected\n");
        return NULL;
    }

    uint32_t id = ++g_call_id;
    send_msg(MSG_LIST_SYMBOLS, id, NULL, 0);

    MsgHeader hdr;
    uint8_t *buf = malloc(BUF_SIZE);
    if (recv_msg_dyn(&hdr, &buf) < 0 || hdr.type != MSG_RETURN) {
        free(buf);
        return NULL;
    }

    int count = 0;
    for (uint32_t i = 0; i < hdr.data_len - 1; i++)
        if (buf[i] == '\0') count++;

    jclass str_cls = (*env)->FindClass(env, "java/lang/String");
    jobjectArray arr = (*env)->NewObjectArray(env, count, str_cls, NULL);

    int idx = 0, start = 0;
    for (uint32_t i = 0; i <= hdr.data_len; i++) {
        if (buf[i] == '\0' && i > (uint32_t)start) {
            jstring s = (*env)->NewStringUTF(env, (char *)buf + start);
            (*env)->SetObjectArrayElement(env, arr, idx++, s);
            (*env)->DeleteLocalRef(env, s);
            start = i + 1;
        }
    }

    free(buf);
    return arr;
}