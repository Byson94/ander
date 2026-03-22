#pragma once
#include <jni.h>
#include <stdint.h>

#define MAX_NATIVES 1024
#define MAX_TRAMPOLINES 512 // must match gen_trampolines.py

typedef struct {
    char symbol[256];
    int  param_count;
} NativeSlot;

extern NativeSlot g_natives[];
extern int        g_native_count;
extern void       *g_trampolines[];

jlong trampoline_call(JNIEnv *env, jobject obj,
                      const char *symbol,
                      uint64_t *args, uint32_t argc);