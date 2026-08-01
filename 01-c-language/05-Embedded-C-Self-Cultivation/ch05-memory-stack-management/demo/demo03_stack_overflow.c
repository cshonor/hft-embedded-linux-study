/*
 * demo03_stack_overflow.c — 栈溢出教学演示（仅 Linux/WSL + GDB）
 *
 * 模式 A（默认）：小缓冲区写越界，覆盖 saved FP/LR → SIGSEGV 或 stack smashing
 * 模式 B（./demo03_stack_overflow r）：每层 1KB 局部数组 + 递归，耗尽栈空间
 */
#include <stdio.h>
#include <string.h>

void smash_frame(void)
{
    volatile char buf[32];
    /* 故意写越界：覆盖相邻栈帧中的返回地址 */
    memset((char *)buf, 'A', 512);
    printf("returning from smash_frame (no protector?)\n");
}

static void eat_stack(int depth)
{
    volatile char frame[1024];
    frame[0] = (char)depth;
    if (depth > 0)
        eat_stack(depth - 1);
}

int main(int argc, char **argv)
{
    if (argc > 1 && argv[1][0] == 'r') {
        printf("recursive stack exhaustion, ulimit -s 4096 recommended\n");
        eat_stack(64);
    } else {
        printf("buffer overflow demo, try -fno-stack-protector vs -fstack-protector-all\n");
        smash_frame();
    }
    printf("main finished\n");
    return 0;
}
