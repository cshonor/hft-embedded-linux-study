/*
 * CSAPP Ch2 §2.1.3 · (unsigned char *) 逐字节读写 endian
 *
 * 对照: chapter-02/notes/section-2.1.1-2.1.3-*.md
 * 步长: code/pointer-stride-demo.c + chapter-03/notes/section-3.8-指针步长详解.md
 *
 * 编译: gcc -Wall -Wextra -std=c11 -o pointer_bytes pointer-and-bytes.c
 */

#include <stdio.h>
#include <stdint.h>

int main(void)
{
    int x = 0x11223344;
    unsigned char *p = (unsigned char *)&x;

    printf("int x = 0x11223344;\n");
    printf("unsigned char* 逐字节 (p++ 每次 +1):\n  ");
    for (size_t i = 0; i < sizeof x; i++)
        printf("%02x ", p[i]);
    printf("\n");

    p[0] = 0x00;
    printf("p[0] = 0x00 后, x = 0x%08x\n", (unsigned)x);

    int arr[] = {0xAABBCCDD, 0x11223344};
    unsigned char *q = (unsigned char *)arr;
    printf("\nint arr[2] 连续 8 字节 (char* 视角):\n  ");
    for (size_t i = 0; i < sizeof arr; i++)
        printf("%02x ", q[i]);
    printf("\n");

    return 0;
}
