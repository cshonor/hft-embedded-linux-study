/*
 * CSAPP Ch3 §3.8 · 指针类型步长 (pointer stride)
 *
 * 对照: chapter-03/notes/section-3.8-指针步长详解.md
 *
 * 编译: gcc -Wall -Wextra -std=c11 -o pointer_stride pointer-stride-demo.c
 */

#include <inttypes.h>
#include <stdio.h>
#include <stdint.h>

static void show_stride(const char *label, void *base, size_t elem_size)
{
    uintptr_t b = (uintptr_t)base;
    printf("%s  base=%" PRIuPTR "  +1 elem => +%zu bytes\n",
           label, b, elem_size);
}

int main(void)
{
    char carr[5] = {0x11, 0x22, 0x33, 0x44, 0x55};
    int iarr[3] = {0xAABBCCDD, 0x11223344, 0x55667788};

    char *pc = carr;
    int *pi = iarr;

    printf("=== char* vs int* 步长 ===\n\n");
    printf("char arr[5]: ");
    for (int i = 0; i < 5; i++)
        printf("%02x ", (unsigned char)carr[i]);
    printf("\n\n");

    printf("pc=%p  pc+1=%p  (差 %td 字节)\n",
           (void *)pc, (void *)(pc + 1), (char *)(pc + 1) - pc);
    printf("pi=%p  pi+1=%p  (差 %td 字节)\n\n",
           (void *)pi, (void *)(pi + 1), (int *)(pi + 1) - pi);

    printf("*(pc+2) = 0x%02x  (第3个char)\n", (unsigned char)*(pc + 2));
    printf("*(pi+1) = 0x%08x  (第2个int)\n\n", (unsigned)*(pi + 1));

    printf("=== 指针运算 vs 地址当整数加 ===\n\n");
    printf("pi+1           => %p\n", (void *)(pi + 1));
    printf("(uintptr_t)pi+1 => %p  ← 不是下一个 int!\n\n",
           (void *)((uintptr_t)pi + 1));

    show_stride("char", carr, sizeof(char));
    show_stride("short", carr, sizeof(short));
    show_stride("int", iarr, sizeof(int));
    show_stride("long", iarr, sizeof(long));

    printf("\nlong* 步长随 ABI: 本机 sizeof(long)=%zu\n", sizeof(long));

    return 0;
}
