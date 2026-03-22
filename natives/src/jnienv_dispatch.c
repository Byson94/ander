#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <jni.h>
#include "ipc.h"
#include "jnienv_dispatch.h"

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
        memcpy(&arr,   args,                             sizeof(arr));
        memcpy(&elems, args + sizeof(arr),               sizeof(elems));
        memcpy(&mode,  args + sizeof(arr)+sizeof(elems), sizeof(mode));
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
        jarray arr;
        jint mode;
        memcpy(&arr,  args,              sizeof(arr));
        memcpy(&mode, args + sizeof(arr), sizeof(mode));

        if (mode != JNI_ABORT) {
            uint8_t *data     = args + sizeof(arr) + sizeof(mode);
            jsize    data_len = args_len - sizeof(arr) - sizeof(mode);

            void *dst = (*env)->GetPrimitiveArrayCritical(env, arr, NULL);
            if (dst) {
                memcpy(dst, data, data_len);
                (*env)->ReleasePrimitiveArrayCritical(env, arr, dst, 0);
            }
        }
        return 0;
    }
    fprintf(stderr, "[AnderBridge] unimplemented JNIENV: %s\n", method);
    return 0;
}

void handle_jnienv_and_respond(JNIEnv *env, uint8_t *data,
                                uint32_t dlen, uint32_t call_id) {
    char    *method   = (char *)data;
    uint8_t *args     = data + strlen(method) + 1;
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

        jsize len = (*env)->GetArrayLength(env, arr);
        void *ptr = (*env)->GetPrimitiveArrayCritical(env, arr, NULL);
        if (!ptr || len == 0) {
            int64_t zero = 0;
            send_msg(MSG_JNIENV_RETURN, call_id, &zero, sizeof(zero));
            return;
        }

        jclass arr_class   = (*env)->GetObjectClass(env, arr);
        jclass long_arr_class  = (*env)->FindClass(env, "[J");
        jclass int_arr_class   = (*env)->FindClass(env, "[I");
        jclass float_arr_class = (*env)->FindClass(env, "[F");

        jsize elem_size = 1; // default byte
        if      ((*env)->IsSameObject(env, arr_class, long_arr_class))  elem_size = 8;
        else if ((*env)->IsSameObject(env, arr_class, int_arr_class))   elem_size = 4;
        else if ((*env)->IsSameObject(env, arr_class, float_arr_class)) elem_size = 4;

        jsize    byte_len = len * elem_size;
        uint32_t total    = sizeof(int64_t) + byte_len;
        uint8_t *resp     = malloc(total);
        int64_t  handle   = (int64_t)(intptr_t)arr;
        memcpy(resp,                  &handle, sizeof(handle));
        memcpy(resp + sizeof(handle), ptr,     byte_len);
        (*env)->ReleasePrimitiveArrayCritical(env, arr, ptr, JNI_ABORT);

        send_msg(MSG_JNIENV_RETURN_DATA, call_id, resp, total);
        free(resp);
        return;
    }
    if (strcmp(method, "NewDirectByteBuffer") == 0) {
        void *arm_addr; jlong capacity;
        memcpy(&arm_addr, args,                  sizeof(arm_addr));
        memcpy(&capacity, args+sizeof(arm_addr), sizeof(capacity));

        uint8_t *pixel_data = args + sizeof(arm_addr) + sizeof(capacity);

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