#pragma once
#include <jni.h>
#include <stdint.h>

void handle_jnienv_and_respond(JNIEnv *env, uint8_t *data,
                                uint32_t dlen, uint32_t call_id);