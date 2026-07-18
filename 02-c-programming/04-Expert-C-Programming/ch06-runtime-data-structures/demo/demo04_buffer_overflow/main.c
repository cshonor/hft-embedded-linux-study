/*
 * 教学演示：栈上相邻变量被越界写入覆盖（不依赖篡改返回地址）。
 * 真实攻击会覆盖 RA；现代编译器 -fstack-protector 会检测并 abort。
 * 对比：gcc -fno-stack-protector 时行为更「裸」。
 */
#include <stdio.h>
#include <string.h>

static void vulnerable(void)
{
    volatile int auth = 0;
    char buf[8];

    printf("before: auth=%d buf@=%p auth@=%p\n", auth, (void *)buf, (void *)&auth);
    /* 故意越界：覆盖栈上相邻的 auth */
    memset(buf, 'A', 16);
    printf("after:  auth=%d (被栈溢出改写)\n", auth);
}

int main(void)
{
    vulnerable();
    return 0;
}
