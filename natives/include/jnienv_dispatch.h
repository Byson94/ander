#pragma once
#include <jni.h>
#include <stdint.h>

#define MAX_DIRECT_BUFS 256

typedef struct {
    int64_t  arm_handle;
    void    *host_ptr;
    jobject  host_jobject;
    int      is_array;
} DirectBuf;

extern DirectBuf g_direct_bufs[];
extern int       g_direct_buf_count;

void handle_jnienv_and_respond(JNIEnv *env, uint8_t *data,
                                uint32_t dlen, uint32_t call_id);