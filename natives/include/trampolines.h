#pragma once
#include <jni.h>
#include <stdint.h>
#include <ffi.h>

#define MAX_NATIVES 1024
// #define MAX_TRAMPOLINES 512 // must match gen_trampolines.py

typedef struct {
    char     symbol[256];
    char     signature[256];
    int      param_count;
    void    *fn_ptr;
    ffi_closure *closure;
    void    *closure_data;
} NativeSlot;

typedef struct {
    int       slot;
    ffi_cif   cif;
    ffi_type **arg_types;
} ClosureData;

extern NativeSlot g_natives[];
extern int        g_native_count;
extern void       *g_trampolines[];

jlong trampoline_call(JNIEnv *env, jobject obj, int slot, uint64_t *args, uint32_t argc);
void* trampoline_create(int slot);