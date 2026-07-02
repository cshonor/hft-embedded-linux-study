/*
 * 02 C 专练 · 指针与内存字节 (K&R / Pointers on C)
 *
 * 配合 01 CSAPP Ch2 读完 §2.1.3 后做 — 练习 (unsigned char *) 视角。
 * CSAPP 官方 demo 在: 01-CSAPP-3rd/code/ch02-endian-and-padding-demo.c
 *
 * 编译:
 *   gcc -Wall -Wextra -std=c11 -o pointer_bytes pointer-and-bytes.c
 */

#include <stdio.h>
#include <stdint.h>

int main(void)
{
    int x = 0x11223344;
    unsigned char *p = (unsigned char *)&x;

    printf("int x = 0x11223344;\n");
    printf("用 unsigned char* 从低地址读:\n  ");
    for (size_t i = 0; i < sizeof x; i++)
        printf("%02x ", p[i]);
    printf("\n");

    /* 改一个字节 — 理解「内存里改 bit，变量值就变」 */
    p[0] = 0x00;
    printf("p[0] = 0x00 后, x = 0x%08x\n", (unsigned)x);

    int arr[] = {0xAABBCCDD, 0x11223344};
    unsigned char *q = (unsigned char *)arr;
    printf("\nint arr[2] 连续 8 字节:\n  ");
    for (size_t i = 0; i < sizeof arr; i++)
        printf("%02x ", q[i]);
    printf("\n");

    return 0;
}
