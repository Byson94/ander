JAVA_HOME ?= /usr/lib/jvm/default
CC        = gcc
CFLAGS    = -shared -fPIC -O2 \
            -I$(JAVA_HOME)/include \
            -I$(JAVA_HOME)/include/linux \
            -I./include
LDFLAGS   = -lffi

SRCS = src/AnderBridge.c src/ipc.c src/jnienv_dispatch.c src/trampolines.c
OUT  = libAnderBridge.so

$(OUT): $(SRCS)
	$(CC) $(CFLAGS) -o $(OUT) $(SRCS) $(LDFLAGS)

clean:
	rm -f $(OUT)