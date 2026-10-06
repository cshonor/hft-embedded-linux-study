/*
 * demo02_char_promotion —— 字符常量的类型 与 整数提升的边界
 *
 * 构建运行：
 *   make -C demo02_char_promotion && ./demo02_char_promotion/main
 *
 * 本 demo 只做一件事：把「变量类型」和「整数提升」两件事分开打印出来，
 * 顺带给出 sizeof / _Generic / static_assert 三种「取证」手段。
 */
#include <assert.h>
#include <stdio.h>

/* 编译期取证：问「这个表达式的类型是什么」——
 * _Generic 的控制表达式会做左值转换，但【不做整数提升】，所以
 * TYPEOF(c) 得到 char，TYPEOF(c + c) 得到 int，正好把两件事区分开。 */
#define TYPEOF(x) _Generic((x),          \
        char: "char",                    \
        signed char: "signed char",      \
        unsigned char: "unsigned char",  \
        short: "short",                  \
        int: "int",                      \
        unsigned int: "unsigned int",    \
        long: "long",                    \
        default: "other")

/* C11 编译期断言：如果字符常量不是 int，这一行直接编不过 */
static_assert(sizeof('c') == sizeof(int), "C 语言里字符常量的类型就是 int");

int main(void)
{
    char c = 'A';

    puts("=== 1. sizeof：字面量 vs 变量 ===");
    printf("sizeof('c')       = %zu   <- 字符常量，类型 int\n", sizeof('c'));
    printf("sizeof(c)         = %zu   <- char 变量，类型 char\n", sizeof(c));
    printf("sizeof(char)      = %zu\n", sizeof(char));
    printf("sizeof(int)       = %zu\n", sizeof(int));
    printf("sizeof((char)'c') = %zu   <- 想要 1 字节就得显式转\n", sizeof((char)'c'));
    printf("sizeof(\"A\")       = %zu   <- 字符串字面量，含 '\\0'\n", sizeof("A"));

    puts("\n=== 2. 类型取证（_Generic）===");
    printf("type of 'c'    = %s\n", TYPEOF('c'));
    printf("type of c      = %s\n", TYPEOF(c));
    printf("type of c + c  = %s\n", TYPEOF(c + c));
    printf("type of c << 1 = %s\n", TYPEOF(c << 1));
    printf("type of c == 0 = %s\n", TYPEOF(c == 0));
    printf("type of ~c     = %s\n", TYPEOF(~c));

    puts("\n=== 3. sizeof 的操作数【不做】整数提升 ===");
    printf("sizeof(c)      = %zu   <- 变量本身，不提升\n", sizeof(c));
    printf("sizeof(+c)     = %zu   <- 一元 + 是表达式，提升发生\n", sizeof(+c));
    printf("sizeof(-c)     = %zu\n", sizeof(-c));
    printf("sizeof(c << 0) = %zu\n", sizeof(c << 0));
    printf("sizeof(c + 0)  = %zu\n", sizeof(c + 0));
    printf("sizeof(c == 0) = %zu\n", sizeof(c == 0));

    puts("\n=== 4. 整数提升救场：窄类型先升到 int 再算 ===");
    {
        unsigned char a = 200, b = 100;
        /* a、b 先提升为 int（-128..127 装不下 200 也不影响，int 装得下），
         * 减法按有符号算得 100；若在 8 位里算，200-100 会以 256 取模 → 100 也对，
         * 真正会翻车的是 a=100,b=200 这种「小减大」——见下 */
        printf("a=200, b=100  ->  a - b        = %d\n", a - b);

        unsigned char x = 100, y = 200;
        printf("x=100, y=200  ->  x - y        = %d   <- 提升为 int，得到负数而非回绕\n", x - y);
        printf("             ->  (unsigned char)(x - y) = %d   <- 截回 8 位才是回绕值\n",
               (unsigned char)(x - y));
    }

    puts("\n=== 5. char 的符号性 与 0xFF 经典坑 ===");
    {
        char ch = (char)0xFF;
        int mask = 0xFF;    /* 用变量接一下，避开编译器的「常量恒假」告警 */
        int neg  = -1;      /* 同理：字面量 -1 在 char 为 unsigned 的平台上会触 -Wtype-limits */

        printf("char is %s on this platform\n", ((char)-1 < 0) ? "signed" : "unsigned");
        printf("(int)ch                    = %d   (0x%08X)\n", (int)ch, (unsigned)(int)ch);
        printf("ch == mask(0xFF)           -> %s   <- 两边都提升为 int 再比\n",
               (ch == mask) ? "true" : "false");
        printf("ch == neg(-1)              -> %s\n", (ch == neg) ? "true" : "false");
        printf("(unsigned char)ch == 0xFF  -> %s   <- 截回 8 位再提升，才对得上\n",
               ((unsigned char)ch == mask) ? "true" : "false");
        printf("  ^ char 的符号性由目标 ABI 决定：arm64 Linux unsigned / x86_64 与 macOS signed\n");
        printf("    同一份代码在两平台上，ch == mask 的结论完全相反（见 2.3.1 §5）\n");

        /* 下面是「裸字面量」写法：结果同样是 false，但编译器会直接告警
         * clang: warning: result of comparison of constant 255 with expression
         *        of type 'char' is always false [-Wtautological-constant-out-of-range-compare]
         * 取消注释即可看到。 */
        /* printf("ch == 0xFF -> %s\n", (ch == 0xFF) ? "true" : "false"); */
    }

    puts("\n=== 6. 可变参数：提升由编译器自动完成 ===");
    {
        char z = 'A';
        printf("printf(\"%%c\", z) = %c      printf(\"%%d\", z) = %d\n", z, z);
        printf("传进 printf 的是提升后的 int，不是 1 字节的 char\n");
        /* 反例：printf("%s", z);  z 提升为 int(65)，不是指针 → UB */
    }

    return 0;
}
