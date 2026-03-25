#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <jni.h>
#include "ipc.h"
#include "jnienv_dispatch.h"
#include "trampolines.h"
#include <ffi.h>

NativeSlot g_natives[MAX_NATIVES];
int        g_native_count = 0;
static uint64_t g_array_token = 0x4100000000000000ULL;

typedef enum {
    ARG_OBJECT,   // L...; or [ arrays (needs jobject resolution)
    ARG_INT,      // I Z B C S
    ARG_LONG,     // J
    ARG_FLOAT,    // F
    ARG_DOUBLE,   // D
} ArgType;

static int parse_signature(const char *sig, ArgType *types, int max_types) {
    // sig is like "(Ljava/nio/Buffer;ILjava/nio/Buffer;II)V"
    int count = 0;
    const char *p = sig + 1;
    while (*p && *p != ')' && count < max_types) {
        if (*p == '[') {
            while (*p == '[') p++;
            if (*p == 'L') { while (*p && *p != ';') p++; }
            types[count++] = ARG_OBJECT;
        } else if (*p == 'L') {
            while (*p && *p != ';') p++;
            types[count++] = ARG_OBJECT;
        } else if (*p == 'I' || *p == 'Z' || *p == 'B' || *p == 'C' || *p == 'S') {
            types[count++] = ARG_INT;
        } else if (*p == 'J') {
            types[count++] = ARG_LONG;
        } else if (*p == 'F') {
            types[count++] = ARG_FLOAT;
        } else if (*p == 'D') {
            types[count++] = ARG_DOUBLE;
        }
        if (*p) p++;
    }
    return count;
}

static ffi_type *sig_char_to_ffi_type(char c) {
    switch (c) {
        case 'F': return &ffi_type_float;
        case 'D': return &ffi_type_double;
        case 'J': return &ffi_type_sint64;
        case 'Z': case 'B': case 'C': case 'S': return &ffi_type_sint32;
        case 'I': return &ffi_type_sint32;
        default:  return &ffi_type_pointer;
    }
}

// Returns the arm_handle to use as args[i], or 0 on failure
static uint64_t transfer_array(JNIEnv *env, jobject jobj, const char elem_char) {
    jsize esize = 1;
    switch (elem_char) {
        case 'J': case 'D': esize = 8; break;
        case 'I': case 'F': esize = 4; break;
        case 'S': case 'C': esize = 2; break;
        default:             esize = 1; break;
    }

    // Check if already registered
    for (int i = 0; i < g_direct_buf_count; i++) {
        if (g_direct_bufs[i].host_jobject != NULL &&
            (*env)->IsSameObject(env, g_direct_bufs[i].host_jobject, jobj)) {
            uint64_t arm_handle = (uint64_t)(uintptr_t)g_direct_bufs[i].arm_handle;

            jsize len = (*env)->GetArrayLength(env, (jarray)jobj);
            if ((*env)->ExceptionCheck(env)) { (*env)->ExceptionClear(env); return arm_handle; }
            jsize byte_len = len * esize;

            void *src = (*env)->GetPrimitiveArrayCritical(env, (jarray)jobj, NULL);
            if (!(*env)->ExceptionCheck(env) && src) {
                uint32_t header_size = sizeof(uint64_t) + sizeof(uint32_t);
                uint8_t *payload = malloc(header_size + byte_len);
                if (payload) {
                    uint32_t blen = (uint32_t)byte_len;
                    memcpy(payload,                    &arm_handle, sizeof(arm_handle));
                    memcpy(payload + sizeof(uint64_t), &blen,       sizeof(uint32_t));
                    memcpy(payload + header_size,       src,        byte_len);
                    (*env)->ReleasePrimitiveArrayCritical(env, (jarray)jobj, src, JNI_ABORT);
                    send_msg(MSG_ARRAY_DATA, 0, payload, header_size + byte_len);
                    free(payload);
                    while (1) {
                        MsgHeader ack_hdr;
                        uint8_t *ack_buf = NULL;
                        if (recv_msg_dyn(&ack_hdr, &ack_buf) < 0) break;
                        if (ack_hdr.type == MSG_ARRAY_ACK) { if (ack_buf) free(ack_buf); break; }
                        fprintf(stderr, "[transfer_array] unexpected msg 0x%02x waiting for ACK\n", ack_hdr.type);
                        if (ack_buf) free(ack_buf);
                    }
                } else {
                    (*env)->ReleasePrimitiveArrayCritical(env, (jarray)jobj, src, JNI_ABORT);
                }
            } else {
                if ((*env)->ExceptionCheck(env)) (*env)->ExceptionClear(env);
            }
            return arm_handle;
        }
    }

    // New array 
    jsize len = (*env)->GetArrayLength(env, (jarray)jobj);
    if ((*env)->ExceptionCheck(env)) { (*env)->ExceptionClear(env); return 0; }
    if (len <= 0) return 0;

    jsize byte_len = len * esize;

    void *src = (*env)->GetPrimitiveArrayCritical(env, (jarray)jobj, NULL);
    if ((*env)->ExceptionCheck(env)) { (*env)->ExceptionClear(env); return 0; }
    if (!src) return 0;

    uint32_t header_size = sizeof(uint64_t) + sizeof(uint32_t);
    uint8_t *payload = malloc(header_size + byte_len);
    if (!payload) {
        (*env)->ReleasePrimitiveArrayCritical(env, (jarray)jobj, src, JNI_ABORT);
        return 0;
    }

    uint64_t arm_handle = g_array_token++;
    uint32_t blen = (uint32_t)byte_len;
    memcpy(payload, &arm_handle, sizeof(arm_handle));
    memcpy(payload + sizeof(uint64_t), &blen, sizeof(uint32_t));
    memcpy(payload + header_size, src, byte_len);

    (*env)->ReleasePrimitiveArrayCritical(env, (jarray)jobj, src, JNI_ABORT);

    // Register on host side
    if (g_direct_buf_count >= MAX_DIRECT_BUFS) {
        for (int i = 0; i < g_direct_buf_count; i++) {
            if (g_direct_bufs[i].is_array) {
                fprintf(stderr, "[transfer_array] table full, evicting idx=%d token=0x%llx\n",
                        i, (unsigned long long)g_direct_bufs[i].arm_handle);
                fflush(stderr);
                (*env)->DeleteGlobalRef(env, g_direct_bufs[i].host_jobject);
                g_direct_bufs[i] = g_direct_bufs[--g_direct_buf_count];
                break;
            }
        }
    }

    if (g_direct_buf_count < MAX_DIRECT_BUFS) {
        jobject gref = (*env)->NewGlobalRef(env, jobj);
        int idx = g_direct_buf_count++;
        g_direct_bufs[idx].host_jobject = gref;
        g_direct_bufs[idx].host_ptr     = NULL;
        g_direct_bufs[idx].arm_handle   = (int64_t)arm_handle;
        g_direct_bufs[idx].is_array     = 1;
    }

    send_msg(MSG_ARRAY_DATA, 0, payload, header_size + byte_len);
    free(payload);

    while (1) {
        MsgHeader ack_hdr;
        uint8_t *ack_buf = NULL;
        if (recv_msg_dyn(&ack_hdr, &ack_buf) < 0) break;
        if (ack_hdr.type == MSG_ARRAY_ACK) { if (ack_buf) free(ack_buf); break; }
        fprintf(stderr, "[transfer_array] unexpected msg 0x%02x waiting for ACK\n", ack_hdr.type);
        if (ack_buf) free(ack_buf);
    }

    return arm_handle;
}

jlong trampoline_call(JNIEnv *env, jobject obj,
                      int slot,
                      uint64_t *args, uint32_t argc) {
    if (g_sock < 0) {
        fprintf(stderr, "[AnderBridge] NOT CONNECTED!\n");
        fflush(stderr);
        return 0;
    }

    const char *symbol = g_natives[slot].symbol;

    //* 32 args is assumed to be good enough for most JNI native param count.
    //* If any error related to that pops up, then this line needs to be replaced.
    ArgType arg_types[32];
    int type_count = parse_signature(g_natives[slot].signature, arg_types, 32);

    const char *sigp = g_natives[slot].signature + 1;

    for (uint32_t i = 0; i < argc; i++) {
        if (i >= (uint32_t)type_count) break;

        // Track element char for arrays
        char elem_char = 'B'; // default
        if (*sigp == '[') {
            sigp++; // skip '['
            elem_char = *sigp;
            if (*sigp == 'L') { while (*sigp && *sigp != ';') sigp++; }
        } else if (*sigp == 'L') {
            while (*sigp && *sigp != ';') sigp++;
        }
        if (*sigp) sigp++; // advance to next arg

        if (arg_types[i] != ARG_OBJECT) continue;

        uint64_t v = args[i];
        int found = 0;

        // 1. Direct match by host_jobject
        for (int j = 0; j < g_direct_buf_count; j++) {
            if ((uint64_t)(uintptr_t)g_direct_bufs[j].host_jobject == v) {
                args[i] = (uint64_t)(uintptr_t)g_direct_bufs[j].arm_handle;
                found = 1;
                break;
            }
        }
        if (found) continue;

        // 2. Try GetDirectBufferAddress
        jobject jobj = (jobject)(uintptr_t)v;
        if (jobj != NULL) {
            void *addr = (*env)->GetDirectBufferAddress(env, jobj);
            if ((*env)->ExceptionCheck(env)) (*env)->ExceptionClear(env);
            if (addr) {
                for (int j = 0; j < g_direct_buf_count; j++) {
                    if (g_direct_bufs[j].host_ptr == addr) {
                        args[i] = (uint64_t)(uintptr_t)g_direct_bufs[j].arm_handle;
                        found = 1;
                        break;
                    }
                }
            }
        }
        if (found) continue;

        // 3. Primitive array (transfer via MSG_ARRAY_DATA)
        if (jobj != NULL) {
            uint64_t handle = transfer_array(env, jobj, elem_char);
            if (handle != 0) {
                args[i] = handle;
                found = 1;
            }
        }

        // 4. Unresolved
        if (!found) {
            fprintf(stderr, "[trampoline] unresolved ARG_OBJECT arg[%u]=0x%llx symbol=%s\n",
                    i, (unsigned long long)v, symbol);
            fflush(stderr);
            args[i] = 0;
        }
    }

    uint32_t sym_len  = strlen(symbol) + 1;
    uint32_t sig_len  = strlen(g_natives[slot].signature) + 1;
    uint32_t args_len = argc * 8;
    uint32_t total    = sym_len + sig_len + args_len;
    uint8_t *payload  = malloc(total);
    memcpy(payload, symbol, sym_len);
    memcpy(payload + sym_len, g_natives[slot].signature, sig_len);
    if (args_len > 0) memcpy(payload + sym_len + sig_len, args, args_len);

    uint32_t id = ++g_call_id;
    send_msg(MSG_CALL, id, payload, total);
    free(payload);

    MsgHeader hdr;
    uint8_t  *buf = malloc(BUF_SIZE);
    jlong result  = 0;

    while (1) {
        if (recv_msg_dyn(&hdr, &buf) < 0) {
            fprintf(stderr, "[AnderBridge] recv_msg failed for id=%u symbol=%s\n", id, symbol);
            fflush(stderr);
            break;
        }
        if (hdr.type == MSG_RETURN && hdr.id == id) {
            memcpy(&result, buf, sizeof(result));
            for (int i = 0; i < g_direct_buf_count; i++) {
                if (g_direct_bufs[i].arm_handle == result) {
                    result = (jlong)(intptr_t)g_direct_bufs[i].host_jobject;
                    break;
                }
            }
            break;
        } else if (hdr.type == MSG_JNIENV && hdr.id == id) {
            handle_jnienv_and_respond(env, buf, hdr.data_len, id);
        } else if (hdr.type == MSG_ERROR) {
            fprintf(stderr, "[AnderBridge] MSG_ERROR id=%u: %s\n", id, (char *)buf);
            fflush(stderr);
            break;
        } else {
            fprintf(stderr, "[AnderBridge] UNEXPECTED msg type=0x%02x id=%u while waiting for id=%u. Dropping...\n",
                    hdr.type, hdr.id, id);
            fflush(stderr);
        }
    }

    free(buf);
    return result;
}

static void closure_handler(ffi_cif *cif, void *ret,
                             void **args, void *userdata) {
    (void)cif;
    ClosureData *cd = (ClosureData *)userdata;
    JNIEnv *env = *(JNIEnv **)args[0];
    // args[0]=env, args[1]=cls, args[2..]=real params

    uint32_t nparams = g_natives[cd->slot].param_count;
    // for (uint32_t i = 0; i < nparams; i++) {
    //     uint64_t v;
    //     memcpy(&v, args[i+2], sizeof(uint64_t));
    // }
    
    // TODO: Probably move raw[32] to not hardcode 32 
    // TODO: And dynamically allocate 
    uint64_t raw[32] = {0};
    const char *p = g_natives[cd->slot].signature + 1;
    for (uint32_t i = 0; i < nparams && i < 32; i++) {
        char c = *p;
        if (c == '[') { while (*p == '[') p++; if (*p == 'L') { while (*p && *p != ';') p++; } c = '['; }
        else if (c == 'L') { while (*p && *p != ';') p++; }
        if (*p) p++;

        if (c == 'F') {
            float f; memcpy(&f, args[i+2], sizeof(float));
            uint32_t bits; memcpy(&bits, &f, sizeof(bits));
            raw[i] = (uint64_t)bits;
        } else if (c == 'D') {
            memcpy(&raw[i], args[i+2], sizeof(uint64_t));
        } else if (c == 'J') {
            memcpy(&raw[i], args[i+2], sizeof(uint64_t));
        } else {
            uint64_t v;
            memcpy(&v, args[i+2], sizeof(uint64_t));
            raw[i] = v;
        }
    }

    jlong result = trampoline_call(env, NULL, cd->slot, raw, nparams);
    memcpy(ret, &result, sizeof(jlong));
}

// Call this instead of registering static trampolines
void* trampoline_create(int slot) {
    ClosureData *cd = calloc(1, sizeof(ClosureData));
    cd->slot = slot;

    // All trampolines have the same CIF shape:
    // (JNIEnv*, jclass, uint64, uint64, ...) -> jlong
    // We use 2 + param_count args, all as pointers/sint64
    uint32_t nparams = g_natives[slot].param_count;
    uint32_t total_args = 2 + nparams;

    cd->arg_types = malloc(total_args * sizeof(ffi_type *));
    if (!cd->arg_types) {
        free(cd);
        return NULL;
    }

    cd->arg_types[0] = &ffi_type_pointer; // JNIEnv*
    cd->arg_types[1] = &ffi_type_pointer; // jclass

    const char *p = g_natives[slot].signature + 1;
    for (uint32_t i = 0; i < nparams; i++) {
        char c = *p;
        if (c == '[') { while (*p == '[') p++; if (*p == 'L') { while (*p && *p != ';') p++; } c = '[';}
        else if (c == 'L') { while (*p && *p != ';') p++; }
        if (*p) p++;

        switch (c) {
            case 'F': cd->arg_types[2+i] = &ffi_type_float;   break;
            case 'D': cd->arg_types[2+i] = &ffi_type_double;  break;
            case 'J': cd->arg_types[2+i] = &ffi_type_sint64;  break;
            case '[':
            case 'L': cd->arg_types[2+i] = &ffi_type_pointer; break;
            default:  cd->arg_types[2+i] = &ffi_type_sint32;  break;
        }
    }

    if (ffi_prep_cif(&cd->cif, FFI_DEFAULT_ABI, total_args,
                     &ffi_type_sint64, cd->arg_types) != FFI_OK) {
        fprintf(stderr, "[trampoline] ffi_prep_cif failed for slot %d\n", slot);
        free(cd->arg_types);
        free(cd);
        return NULL;
    }

    void *fn_ptr;
    ffi_closure *closure = ffi_closure_alloc(sizeof(ffi_closure), &fn_ptr);
    if (!closure) {
        fprintf(stderr, "[trampoline] ffi_closure_alloc failed\n");
        free(cd->arg_types);
        free(cd);
        return NULL;
    }

    if (ffi_prep_closure_loc(closure, &cd->cif, closure_handler,
                             cd, fn_ptr) != FFI_OK) {
        fprintf(stderr, "[trampoline] ffi_prep_closure_loc failed\n");
        ffi_closure_free(closure);
        free(cd->arg_types);
        free(cd);
        return NULL;
    }

    g_natives[slot].closure      = closure;
    g_natives[slot].closure_data = cd;
    g_natives[slot].fn_ptr       = fn_ptr;
    return fn_ptr;
}