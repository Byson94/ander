#!/bin/bash
set -e

JAVA_HOME=${JAVA_HOME:-/usr/lib/jvm/default}

gcc -shared -fPIC -O2 \
    -I"$JAVA_HOME/include" \
    -I"$JAVA_HOME/include/linux" \
    -I"./include" \
    -o ./libAnderBridge.so \
    src/AnderBridge.c \
    src/ipc.c \
    src/jnienv_dispatch.c \
    src/trampolines.c

echo "Built libAnderBridge.so"