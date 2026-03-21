#!/usr/bin/env python3
count = 512
print(f"#define MAX_TRAMPOLINES_COUNT {count}")
print()
for i in range(count):
    print(f"MAKE_TRAMPOLINE({i})")
print()
print("static void *g_trampolines[] = {")
for i in range(count):
    print(f"    trampoline_{i},")
print("};")
print(f"#define MAX_TRAMPOLINES (sizeof(g_trampolines)/sizeof(g_trampolines[0]))")