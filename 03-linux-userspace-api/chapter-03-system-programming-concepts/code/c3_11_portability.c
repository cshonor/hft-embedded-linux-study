/* TLPI 第 03 章 §3.6.3 —— 杂项可移植性：编译器/平台宏、字节序、有符号性、右移
 *
 * 这些「不定行为」是嵌入式/HFT 里最常见的静默 bug 来源：在 x86 上跑得好好的，
 * 交叉编到 ARM 就换了一套结果。全部用编译期宏 + 实测把它钉死。
 *
 * 编译：gcc -O0 -Wall -Wextra -o c3_11 c3_11_portability.c
 */
#define _GNU_SOURCE
#include <stddef.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>

/* 一个有「洞」的结构体，用来量对齐填充 */
struct padded {
    char a;
    int  b;
    char c;
};

static void flag(const char *name, int on, const char *note)
{
    printf("  %-22s %-4s %s\n", name, on ? "YES" : "NO", note);
}

int main(void)
{
    printf("=== ① 平台/编译器宏（条件编译的判据）===\n");
#if defined(__linux__)
    flag("__linux__", 1, "= Linux 内核 ABI（不是「有 /proc」之类的功能判断）");
#else
    flag("__linux__", 0, "");
#endif
#if defined(__unix__) || defined(__unix)
    flag("__unix__", 1, "= 类 UNIX 系统");
#else
    flag("__unix__", 0, "");
#endif
#if defined(__x86_64__)
    flag("__x86_64__", 1, "64 位 x86");
#else
    flag("__x86_64__", 0, "");
#endif
#if defined(__aarch64__)
    flag("__aarch64__", 1, "64 位 ARM（嵌入式目标常见）");
#else
    flag("__aarch64__", 0, "");
#endif
#if defined(__ARM_EABI__)
    flag("__ARM_EABI__", 1, "32 位 ARM EABI");
#else
    flag("__ARM_EABI__", 0, "");
#endif
#if defined(__GNUC__) && !defined(__clang__)
    flag("__GNUC__", 1, "真 GCC（clang 也会定义 __GNUC__！要额外排除 __clang__）");
#else
    flag("__GNUC__", 1, "clang 伪装成的 GCC");
#endif
#if defined(__SIZEOF_POINTER__)
    printf("  __SIZEOF_POINTER__     %zu 字节\n", (size_t) __SIZEOF_POINTER__);
#endif
#if defined(__SIZEOF_LONG__)
    printf("  __SIZEOF_LONG__        %zu 字节\n", (size_t) __SIZEOF_LONG__);
#endif
#if defined(__BYTE_ORDER__)
    printf("  __BYTE_ORDER__         %s\n",
           __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__ ? "LITTLE_ENDIAN（小端）" :
           __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__ ? "BIG_ENDIAN（大端）" : "PDP/未知");
#endif
#if defined(__ORDER_BIG_ENDIAN__)
    printf("  __ORDER_BIG_ENDIAN__   = %d\n", __ORDER_BIG_ENDIAN__);
    printf("  __ORDER_LITTLE_ENDIAN__= %d\n", __ORDER_LITTLE_ENDIAN__);
#endif
#if defined(__FLOAT_WORD_ORDER__)
    printf("  __FLOAT_WORD_ORDER__   %s（浮点的字节序可与整数不同！）\n",
           __FLOAT_WORD_ORDER__ == __ORDER_LITTLE_ENDIAN__ ? "LITTLE_ENDIAN" : "BIG_ENDIAN");
#endif

    printf("\n=== ② 运行期验证字节序（宏之外还得看真实内存布局）===\n");
    uint32_t v = 0x01020304u;
    unsigned char b[4];
    memcpy(b, &v, 4);
    printf("  uint32_t v = 0x01020304，逐字节 = %02x %02x %02x %02x → %s端\n",
           b[0], b[1], b[2], b[3], (b[0] == 0x04) ? "小" : "大");
    printf("  ↑ 网络字节序固定是「大端」，所以 htonl()/ntohl() 在小端机上真的会翻字节\n");

    printf("\n=== ③ char 到底有没有符号：这是可移植性最阴的坑 ===\n");
    char c = 0xFF;
    unsigned char uc = 0xFF;
    printf("  (char)0xFF 转 int = %d   → %s\n", (int) c,
           (int) c < 0 ? "char 有符号（x86/aarch64 Linux 默认）" : "char 无符号");
    printf("  (unsigned char)0xFF 转 int = %d\n", (int) uc);
    printf("  危害：char c = getchar(); if (c == 0xFF) ... 在无符号平台永远不成立；\n");
    printf("        而 if (c == EOF) 在有符号平台可能把合法的 0xFF 字节当成 EOF。\n");
    printf("  正确：getchar 的返回值必须装进 int；只处理字节时用 uint8_t / unsigned char\n");

    printf("\n=== ④ 负数右移与整数除法：实现定义 vs 未定义 ===\n");
    int neg = -9;
    printf("  -9 / 2  = %d   （C99 起规定「向零截断」，所以是 -4，不是 -5）\n", neg / 2);
    printf("  -9 %% 2  = %d   （余数符号跟随被除数，所以是 -1）\n", neg % 2);
    printf("  -9 >> 1 = %d   （有符号右移是实现定义：算术移位给 -5，逻辑移位给一个正数）\n",
           neg >> 1);
    printf("  ↑ x86/ARM 的算术移位给 -5；写代码时别依赖它，要么用除法，要么保证操作数无符号\n");

    printf("\n=== ⑤ 结构体内存布局：别假设「成员依次紧排」===\n");
    printf("  struct{char;int;char} sizeof = %zu（不是 6：int 要 4 字节对齐 → 中间补 3、尾部补 3）\n",
           sizeof(struct padded));
    printf("  a 偏移 %zu, b 偏移 %zu, c 偏移 %zu\n",
           offsetof(struct padded, a), offsetof(struct padded, b),
           offsetof(struct padded, c));
    printf("  ↑ 跨平台传结构体（网络包、共享内存、驱动 ioctl）必须显式定序 + 对齐：\n");
    printf("    用 uint8_t/uint32_t、按大到小排成员、必要时 __attribute__((packed))\n");
    return 0;
}
