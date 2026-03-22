#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <jni.h>
#include "ipc.h"
#include "jnienv_dispatch.h"
#include "trampolines.h"

NativeSlot g_natives[MAX_NATIVES];
int        g_native_count = 0;

jlong trampoline_call(JNIEnv *env, jobject obj,
                      const char *symbol,
                      uint64_t *args, uint32_t argc) {
    fprintf(stderr, "[AnderBridge] trampoline_call: %s g_sock=%d\n", symbol, g_sock);
    fflush(stderr);
    if (g_sock < 0) {
        fprintf(stderr, "[AnderBridge] NOT CONNECTED!\n");
        fflush(stderr);
        return 0;
    }

    fprintf(stderr, "[AnderBridge] trampoline call args: ");
    for (uint32_t i = 0; i < argc; i++)
        fprintf(stderr, "[%u]=0x%llx ", i, (unsigned long long)args[i]);
    fprintf(stderr, "\n");
    fflush(stderr);

    uint32_t sym_len  = strlen(symbol) + 1;
    uint32_t args_len = argc * 8;
    uint32_t total    = sym_len + args_len;
    uint8_t *payload  = malloc(total);
    memcpy(payload, symbol, sym_len);
    if (args_len > 0) memcpy(payload + sym_len, args, args_len);

    uint32_t id = ++g_call_id;
    fprintf(stderr, "[AnderBridge] sending MSG_CALL id=%u symbol=%s argc=%u\n", id, symbol, argc);
    fflush(stderr);
    send_msg(MSG_CALL, id, payload, total);
    free(payload);

    MsgHeader hdr;
    uint8_t  *buf = malloc(BUF_SIZE);
    jlong result  = 0;

    while (1) {
        fprintf(stderr, "[AnderBridge] waiting for response id=%u\n", id);
        fflush(stderr);

        if (recv_msg_dyn(&hdr, &buf) < 0) {
            fprintf(stderr, "[AnderBridge] recv_msg failed for id=%u symbol=%s\n", id, symbol);
            fflush(stderr);
            break;
        }

        fprintf(stderr, "[AnderBridge] got msg type=0x%02x id=%u data_len=%u (waiting for id=%u)\n",
                hdr.type, hdr.id, hdr.data_len, id);
        fflush(stderr);

        if (hdr.type == MSG_RETURN && hdr.id == id) {
            memcpy(&result, buf, sizeof(result));
            fprintf(stderr, "[AnderBridge] MSG_RETURN id=%u result=0x%llx\n", id, (unsigned long long)result);
            fflush(stderr);
            break;
        } else if (hdr.type == MSG_JNIENV && hdr.id == id) {
            fprintf(stderr, "[AnderBridge] MSG_JNIENV id=%u dispatching\n", id);
            fflush(stderr);
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

    fprintf(stderr, "[AnderBridge] trampoline_call done: %s -> 0x%llx\n", symbol, (unsigned long long)result);
    fflush(stderr);
    free(buf);
    return result;
}

#define MAKE_TRAMPOLINE(N) \
static jlong trampoline_##N(JNIEnv *env, jobject obj, \
    uint64_t a0, uint64_t a1, uint64_t a2, uint64_t a3, \
    uint64_t a4, uint64_t a5, uint64_t a6, uint64_t a7) { \
    uint64_t args[] = {a0,a1,a2,a3,a4,a5,a6,a7}; \
    return trampoline_call(env, obj, g_natives[N].symbol, \
                           args, g_natives[N].param_count); \
}

#include "trampolines_generated.h"