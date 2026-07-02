/*
 * CSAPP Ch2 · §2.1.2 sizeof / ABI + §2.1.3 字节序 + Ch3 预告 padding
 *
 * 编译:
 *   gcc -Wall -Wextra -std=c11 -o ch02_demo ch02-endian-and-padding-demo.c
 * 运行:
 *   ./ch02_demo
 *
 * 对照笔记:
 *   chapter-02-representing-information/notes/section-2.1.2-数据大小与sizeof.md
 *   chapter-02-representing-information/notes/section-2.1.3-寻址与字节序.md
 */

#include <stdio.h>
#include <stdint.h>
#include <stddef.h>

static void print_bytes(const char *label, const void *p, size_t n)
{
    const unsigned char *bytes = (const unsigned char *)p;

    printf("%s (%zu bytes @ %p): ", label, n, p);
    for (size_t i = 0; i < n; i++)
        printf("%02x ", bytes[i]);
    printf("\n");
}

static void demo_sizeof(void)
{
    printf("=== 1. sizeof / ABI (§2.1.2) ===\n\n");
    printf("sizeof 是编译期常量，随目标架构+ABI 变化：\n\n");
    printf("  char=%zu  short=%zu  int=%zu  long=%zu  void*=%zu\n",
           sizeof(char), sizeof(short), sizeof(int),
           sizeof(long), sizeof(void *));
    printf("  int32_t=%zu  int64_t=%zu  size_t=%zu\n\n",
           sizeof(int32_t), sizeof(int64_t), sizeof(size_t));

#if defined(_WIN64)
    printf("  当前: Windows x64 → 常见 LLP64 → sizeof(long) 常为 4\n");
#elif defined(__LP64__)
    printf("  当前: LP64 → sizeof(long) 常为 8\n");
#else
    printf("  当前: 32 位或特殊 ABI → 请对照笔记 ABI 表\n");
#endif
    printf("\n  协议字段请用 int32_t/uint64_t，别用 int/long。\n\n");
}

static void demo_endian(void)
{
    int a = 0x11223344;
    uint32_t u = 0x12345678;

    printf("=== 2. Byte order (§2.1.3) ===\n\n");
    printf("int a = 0x11223344;\n");
    print_bytes("  &a", &a, sizeof(a));
    printf("  小端(x86): 44 33 22 11   大端: 11 22 33 44\n\n");

    printf("uint32_t u = 0x12345678;\n");
    print_bytes("  &u", &u, sizeof(u));
    printf("  小端: 78 56 34 12   大端: 12 34 56 78\n\n");

    const char *s = "1234";
    printf("字符串 \"1234\" (书写顺序进内存，勿与整数 endian 混):\n");
    print_bytes("  s", s, 4);
    printf("  字符: %c %c %c %c\n\n", s[0], s[1], s[2], s[3]);
}

struct with_padding {
    char c;
    int i;
    char c2;
};

static void demo_padding(void)
{
    struct with_padding s;

    printf("=== 3. Struct padding (Ch3 §3.9 预告) ===\n\n");
    printf("struct { char c; int i; char c2; }:\n");
    printf("  sizeof = %zu  (直觉 6，实际更大)\n", sizeof(s));
    printf("  offsetof(c)=%zu  offsetof(i)=%zu  offsetof(c2)=%zu\n",
           offsetof(struct with_padding, c),
           offsetof(struct with_padding, i),
           offsetof(struct with_padding, c2));
    print_bytes("  raw bytes", &s, sizeof(s));
    printf("\n  char 与 int 之间 3 字节 padding；末尾也可能补 padding。\n");
}

int main(void)
{
    demo_sizeof();
    demo_endian();
    demo_padding();
    return 0;
}
