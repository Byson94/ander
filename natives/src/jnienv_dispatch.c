#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <jni.h>
#include "ipc.h"
#include "jnienv_dispatch.h"

DirectBuf g_direct_bufs[MAX_DIRECT_BUFS];
int       g_direct_buf_count = 0;

// Token Lookup
static jarray find_real_arr(int64_t arm_token) {
    for (int i = 0; i < g_direct_buf_count; i++) {
        if (g_direct_bufs[i].arm_handle == arm_token) {
            return (jarray)(uintptr_t)g_direct_bufs[i].host_jobject;
        }
    }
    return NULL;
}

void handle_jnienv_and_respond(JNIEnv *env, uint8_t *data,
                                uint32_t dlen, uint32_t call_id) {
    char    *method   = (char *)data;
    uint8_t *args     = data + strlen(method) + 1;
    uint32_t args_len = dlen - (strlen(method) + 1);

    fprintf(stderr, "[jnienv_dispatch] method=%s\n", method);
    fflush(stderr);

    // == Non-array methods ==
    if (strcmp(method, "FindClass") == 0) {
        char *name = (char *)args;
        jclass cls = (*env)->FindClass(env, name);
        int64_t result = (int64_t)(intptr_t)cls;
        send_msg(MSG_JNIENV_RETURN, call_id, &result, sizeof(result));
        return;
    }
    if (strcmp(method, "GetMethodID") == 0) {
        uint64_t wire;
        memcpy(&wire, args, sizeof(wire));
        jclass cls = (jclass)(uintptr_t)wire;
        const char *name = (const char *)(args + sizeof(uint64_t));
        const char *sig  = name + strlen(name) + 1;
        if (!cls) {
            int64_t result = 0;
            send_msg(MSG_JNIENV_RETURN, call_id, &result, sizeof(result));
            return;
        }
        jmethodID mid = (*env)->GetMethodID(env, cls, name, sig);
        int64_t result = (int64_t)(uintptr_t)mid;
        send_msg(MSG_JNIENV_RETURN, call_id, &result, sizeof(result));
        return;
    }
    if (strcmp(method, "GetObjectClass") == 0) {
        uint64_t wire;
        memcpy(&wire, args, sizeof(wire));
        jobject obj = (jobject)(uintptr_t)wire;
        if (!obj) {
            int64_t result = 0;
            send_msg(MSG_JNIENV_RETURN, call_id, &result, sizeof(result));
            return;
        }
        jclass cls = (*env)->GetObjectClass(env, obj);
        int64_t result = (int64_t)(uintptr_t)cls;
        send_msg(MSG_JNIENV_RETURN, call_id, &result, sizeof(result));
        return;
    }
    if (strcmp(method, "NewGlobalRef") == 0) {
        uint64_t wire;
        memcpy(&wire, args, sizeof(wire));
        jobject obj = (jobject)(uintptr_t)wire;
        if (!obj) {
            int64_t result = 0;
            send_msg(MSG_JNIENV_RETURN, call_id, &result, sizeof(result));
            return;
        }
        jobject ref = (*env)->NewGlobalRef(env, obj);
        int64_t result = (int64_t)(uintptr_t)ref;
        send_msg(MSG_JNIENV_RETURN, call_id, &result, sizeof(result));
        return;
    }
    if (strcmp(method, "ExceptionCheck") == 0) {
        int64_t result = (int64_t)(*env)->ExceptionCheck(env);
        send_msg(MSG_JNIENV_RETURN, call_id, &result, sizeof(result));
        return;
    }
    if (strcmp(method, "ExceptionClear") == 0) {
        (*env)->ExceptionClear(env);
        int64_t zero = 0;
        send_msg(MSG_JNIENV_RETURN, call_id, &zero, sizeof(zero));
        return;
    }
    if (strcmp(method, "NewStringUTF") == 0) {
        char *str = (char *)args;
        jstring s = (*env)->NewStringUTF(env, str);
        int64_t result = (int64_t)(intptr_t)s;
        send_msg(MSG_JNIENV_RETURN, call_id, &result, sizeof(result));
        return;
    }
    if (strcmp(method, "NewFloatArray") == 0) {
        jsize len; memcpy(&len, args, sizeof(len));
        jfloatArray arr = (*env)->NewFloatArray(env, len);
        int64_t result = (int64_t)(intptr_t)arr;
        send_msg(MSG_JNIENV_RETURN, call_id, &result, sizeof(result));
        return;
    }

    // == Buffer methods ==
    if (strcmp(method, "NewDirectByteBuffer") == 0) {
        void *arm_addr; jlong capacity;
        memcpy(&arm_addr, args,                  sizeof(arm_addr));
        memcpy(&capacity, args+sizeof(arm_addr), sizeof(capacity));

        void *host_buf = malloc(capacity);
        jobject direct_buf = (*env)->NewDirectByteBuffer(env, host_buf, capacity);
        jobject global_ref = (*env)->NewGlobalRef(env, direct_buf);

        int found = -1;
        for (int i = 0; i < g_direct_buf_count; i++) {
            if (g_direct_bufs[i].arm_handle == (int64_t)(intptr_t)arm_addr) {
                found = i; break;
            }
        }
        if (found >= 0) {
            (*env)->DeleteGlobalRef(env, g_direct_bufs[found].host_jobject);
            free(g_direct_bufs[found].host_ptr);
            g_direct_bufs[found].host_ptr     = host_buf;
            g_direct_bufs[found].host_jobject = global_ref;
        } else if (g_direct_buf_count < MAX_DIRECT_BUFS) {
            int idx = g_direct_buf_count++;
            g_direct_bufs[idx].arm_handle   = (int64_t)(intptr_t)arm_addr;
            g_direct_bufs[idx].host_ptr     = host_buf;
            g_direct_bufs[idx].host_jobject = global_ref;
        }
        fprintf(stderr, "[jnienv_dispatch] NewDirectByteBuffer: arm=%p jobject=%p\n",
                arm_addr, (void*)global_ref);
        fflush(stderr);

        int64_t result = (int64_t)(intptr_t)arm_addr;
        send_msg(MSG_JNIENV_RETURN, call_id, &result, sizeof(result));
        return;
    }
    if (strcmp(method, "GetDirectBufferAddress") == 0) {
        int64_t handle;
        memcpy(&handle, args, sizeof(handle));

        for (int i = 0; i < g_direct_buf_count; i++) {
            if (g_direct_bufs[i].arm_handle == handle) {
                int64_t ptr = (int64_t)(intptr_t)g_direct_bufs[i].host_ptr;
                send_msg(MSG_JNIENV_RETURN, call_id, &ptr, sizeof(ptr));
                return;
            }
        }
        
        send_msg(MSG_JNIENV_RETURN, call_id, &handle, sizeof(handle));
        return;
    }

    // == Array methods ==
    if (strcmp(method, "GetArrayLength") == 0) {
        int64_t arm_token;
        memcpy(&arm_token, args, sizeof(arm_token));
        jarray real_arr = find_real_arr(arm_token);
        if (!real_arr) {
            fprintf(stderr, "[jnienv_dispatch] GetArrayLength: no entry for 0x%llx\n",
                    (unsigned long long)arm_token);
            int64_t zero = 0;
            send_msg(MSG_JNIENV_RETURN, call_id, &zero, sizeof(zero));
            return;
        }
        int64_t result = (int64_t)(*env)->GetArrayLength(env, real_arr);
        send_msg(MSG_JNIENV_RETURN, call_id, &result, sizeof(result));
        return;
    }
    if (strcmp(method, "GetPrimitiveArrayCritical") == 0) {
        int64_t arm_token;
        memcpy(&arm_token, args, sizeof(arm_token));
        jarray real_arr = find_real_arr(arm_token);
        if (!real_arr) {
            fprintf(stderr, "[jnienv_dispatch] GetPrimitiveArrayCritical: no entry for 0x%llx\n",
                    (unsigned long long)arm_token);
            int64_t zero = 0;
            send_msg(MSG_JNIENV_RETURN, call_id, &zero, sizeof(zero));
            return;
        }

        jsize len = (*env)->GetArrayLength(env, real_arr);
        void *ptr = (*env)->GetPrimitiveArrayCritical(env, real_arr, NULL);
        if (!ptr || len == 0) {
            int64_t zero = 0;
            send_msg(MSG_JNIENV_RETURN, call_id, &zero, sizeof(zero));
            return;
        }

        // Determine element size
        jclass arr_class       = (*env)->GetObjectClass(env, real_arr);
        jclass long_arr_class  = (*env)->FindClass(env, "[J");
        jclass int_arr_class   = (*env)->FindClass(env, "[I");
        jclass float_arr_class = (*env)->FindClass(env, "[F");
        jsize elem_size = 1;
        if      ((*env)->IsSameObject(env, arr_class, long_arr_class))  elem_size = 8;
        else if ((*env)->IsSameObject(env, arr_class, int_arr_class))   elem_size = 4;
        else if ((*env)->IsSameObject(env, arr_class, float_arr_class)) elem_size = 4;

        jsize    byte_len = len * elem_size;
        uint32_t total    = sizeof(int64_t) + byte_len;
        uint8_t *resp     = malloc(total);
        memcpy(resp, &arm_token, sizeof(arm_token));
        memcpy(resp + sizeof(arm_token), ptr, byte_len);

        (*env)->ReleasePrimitiveArrayCritical(env, real_arr, ptr, JNI_ABORT);
        send_msg(MSG_JNIENV_RETURN_DATA, call_id, resp, total);
        free(resp);
        return;
    }
    if (strcmp(method, "ReleasePrimitiveArrayCritical") == 0) {
        int64_t arm_token;
        jint mode;
        memcpy(&arm_token, args, sizeof(arm_token));
        memcpy(&mode, args + sizeof(arm_token), sizeof(mode));

        fprintf(stderr, "[jnienv_dispatch] ReleasePrimitiveArrayCritical: token=0x%llx mode=%d count_before=%d\n",
                (unsigned long long)arm_token, (int)mode, g_direct_buf_count);
        fflush(stderr);

        int found_idx = -1;
        for (int i = 0; i < g_direct_buf_count; i++) {
            if (g_direct_bufs[i].arm_handle == arm_token) {
                found_idx = i;
                break;
            }
        }

        if (found_idx >= 0 && mode != JNI_ABORT) {
            jarray real_arr = find_real_arr(arm_token);
            if (real_arr) {
                uint8_t *data_ptr = args + sizeof(arm_token) + sizeof(mode);
                jsize    data_len = args_len - sizeof(arm_token) - sizeof(mode);
                void *dst = (*env)->GetPrimitiveArrayCritical(env, real_arr, NULL);
                if (dst) {
                    memcpy(dst, data_ptr, data_len);
                    (*env)->ReleasePrimitiveArrayCritical(env, real_arr, dst, 0);
                }
            }
        }

        if (found_idx >= 0) {
            if (g_direct_bufs[found_idx].is_array) {
                (*env)->DeleteGlobalRef(env, g_direct_bufs[found_idx].host_jobject);
                g_direct_bufs[found_idx] = g_direct_bufs[--g_direct_buf_count];
            }
        }

        int64_t zero = 0;
        send_msg(MSG_JNIENV_RETURN, call_id, &zero, sizeof(zero));
        return;
    }
    if (strcmp(method, "GetFloatArrayElements") == 0) {
        int64_t arm_token;
        memcpy(&arm_token, args, sizeof(arm_token));
        jarray real_arr = find_real_arr(arm_token);
        if (!real_arr) {
            int64_t zero = 0;
            send_msg(MSG_JNIENV_RETURN, call_id, &zero, sizeof(zero));
            return;
        }
        jfloat *elems = (*env)->GetFloatArrayElements(env, (jfloatArray)real_arr, NULL);
        int64_t result = (int64_t)(intptr_t)elems;
        send_msg(MSG_JNIENV_RETURN, call_id, &result, sizeof(result));
        return;
    }
    if (strcmp(method, "ReleaseFloatArrayElements") == 0) {
        int64_t arm_token; jfloat *elems; jint mode;
        memcpy(&arm_token, args,                         sizeof(arm_token));
        memcpy(&elems,     args + sizeof(arm_token),     sizeof(elems));
        memcpy(&mode,      args + sizeof(arm_token) + sizeof(elems), sizeof(mode));
        jarray real_arr = find_real_arr(arm_token);
        if (real_arr)
            (*env)->ReleaseFloatArrayElements(env, (jfloatArray)real_arr, elems, mode);
        int64_t zero = 0;
        send_msg(MSG_JNIENV_RETURN, call_id, &zero, sizeof(zero));
        return;
    }
    if (strcmp(method, "SetFloatArrayRegion") == 0) {
        int64_t arm_token; jsize start, len;
        memcpy(&arm_token, args,                         sizeof(arm_token));
        memcpy(&start,     args + sizeof(arm_token),     sizeof(start));
        memcpy(&len,       args + sizeof(arm_token) + sizeof(start), sizeof(len));
        jfloat *buf_ptr = (jfloat *)(args + sizeof(arm_token) + sizeof(start) + sizeof(len));
        jarray real_arr = find_real_arr(arm_token);
        if (real_arr)
            (*env)->SetFloatArrayRegion(env, (jfloatArray)real_arr, start, len, buf_ptr);
        int64_t zero = 0;
        send_msg(MSG_JNIENV_RETURN, call_id, &zero, sizeof(zero));
        return;
    }

    fprintf(stderr, "[jnienv_dispatch] unimplemented JNIENV: %s\n", method);
    fflush(stderr);
    int64_t zero = 0;
    send_msg(MSG_JNIENV_RETURN, call_id, &zero, sizeof(zero));
}