/*
 * CSAPP Ch2 风格 · 小端/大端 + 结构体 padding
 *
 * 编译（任选）:
 *   gcc -Wall -Wextra -std=c11 -o endian_demo endian_and_padding_demo.c
 *   clang -Wall -Wextra -std=c11 -o endian_demo endian_and_padding_demo.c
 *
 * 运行: ./endian_demo   (Windows: endian_demo.exe)
 */

#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>

/* ---------- 1. 多字节数值在内存里的字节序 ---------- */

static void print_bytes(const char *label, const void *p, size_t n)
{
    const unsigned char *bytes = (const unsigned char *)p;

    printf("%s (%zu bytes @ %p): ", label, n, p);
    for (size_t i = 0; i < n; i++)
        printf("%02x ", bytes[i]);
    printf("\n");
    printf("  低地址 %p → 高地址 %p\n", (void *)bytes, (void *)(bytes + n - 1));
}

static void demo_endian(void)
{
    int a = 0x11223344;
    uint32_t u = 0x12345678;

    printf("=== 1. Byte order (endian) ===\n\n");
    printf("int a = 0x11223344;\n");
    print_bytes("  &a", &a, sizeof(a));

    printf("\n若第一个字节是 0x44 → 小端 (Little-Endian, x86 常见)\n");
    printf("若第一个字节是 0x11 → 大端 (Big-Endian)\n\n");

    printf("uint32_t u = 0x12345678;\n");
    print_bytes("  &u", &u, sizeof(u));
    printf("  小端期望: 78 56 34 12\n");
    printf("  大端期望: 12 34 56 78\n\n");

    /* 和字符串对比：字符按书写顺序进内存，不像小端整数 */
    const char *s = "1234";
    printf("字符串 \"1234\" (字符顺序 ≈ 大端的「高位/左侧在前」):\n");
    print_bytes("  s", s, 4);
    printf("  字符: %c %c %c %c\n\n", s[0], s[1], s[2], s[3]);
}

/* ---------- 2. 结构体 padding / alignment ---------- */

struct unpadded_guess {
    char c;
    int i;
    char c2;
}; /* 直觉 1+4+1=6，实际因 padding 更大 */

struct packed_example {
    char c;
    char pad[3]; /* 手动标出编译器通常会隐式插入的 3 字节 */
    int i;
    char c2;
    char pad2[3];
};

static void demo_padding(void)
{
    struct unpadded_guess s;

    printf("=== 2. Struct padding (alignment) ===\n\n");
    printf("struct { char c; int i; char c2; }:\n");
    printf("  sizeof = %zu (不是 6，因为有 padding)\n", sizeof(s));
    printf("  offsetof(c)  = %zu\n", offsetof(struct unpadded_guess, c));
    printf("  offsetof(i)  = %zu  ← int 从 4 的倍数地址开始\n",
           offsetof(struct unpadded_guess, i));
    printf("  offsetof(c2) = %zu\n", offsetof(struct unpadded_guess, c2));

    print_bytes("  内存布局", &s, sizeof(s));
    printf("\n  char 后补 3 字节 padding，让 int 对齐到 4 字节边界；\n");
    printf("  末尾再补 padding，使 struct 总大小为对齐单位的倍数。\n");
}

int main(void)
{
    demo_endian();
    demo_padding();
    return 0;
}
