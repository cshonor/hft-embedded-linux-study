// 3.2.5 —— cctype 实测：char 的符号性、负值 UB、表查找 vs 手写比较
// 编译：clang++ -std=c++17 -O2 examples/3.2.5-1-cctype.cpp -o cctype && ./cctype
#include <cctype>
#include <chrono>
#include <cstdio>
#include <limits>
#include <string>

int main() {
    printf("① char 的符号性（决定那个 UB 是否真实存在）\n");
    printf("  std::numeric_limits<char>::is_signed = %d\n",
           (int)std::numeric_limits<char>::is_signed);
#ifdef __CHAR_UNSIGNED__
    printf("  __CHAR_UNSIGNED__ 已定义 → char 是 unsigned\n");
#else
    printf("  __CHAR_UNSIGNED__ 未定义 → char 是 signed\n");
#endif
    char c = (char)0x80;
    printf("  char c = 0x80 → (int)c = %d    ← 负数才是 UB 风险\n", (int)c);
    printf("  ※ macOS/Darwin ABI 上 char 恒为 signed（哪怕 arm64）；\n"
           "    AArch64 AAPCS 才是 unsigned。别照抄\"ARM 上 char 无符号\"的说法。\n");

    printf("② 传负值给 cctype（标准说 UB）\n");
    printf("  isdigit(-1)   = %d\n", isdigit(-1));
    printf("  isalpha(-128) = %d\n", isalpha(-128));
    printf("  isdigit(128)  = %d   ← 128 是合法值（unsigned char 的值）\n", isdigit(128));
    printf("  isdigit('7')  = %d   ← 返回「非零」表示真，不一定是 1\n", isdigit('7'));
    printf("  ※ 实测 macOS 上不崩（返回 0），但 glibc 的实现是 __ctype_b 表索引，\n"
           "    负索引会读到表外 —— 换平台就可能是越界读。别依赖实测结果。\n");

    printf("③ 正确写法：先转 unsigned char\n");
    const char* s = "IF2406";
    for (const char* p = s; *p; ++p) {
        unsigned char uc = static_cast<unsigned char>(*p);
        printf("  '%c' → isalnum=%d\n", *p, isalnum(uc) != 0);
    }

    printf("④ 表查找 vs 手写比较（1 亿次）\n");
    const int N = 100'000'000;
    volatile long sink = 0;
    {
        auto t0 = std::chrono::steady_clock::now();
        long hits = 0;
        for (int i = 0; i < N; ++i) {
            unsigned char ch = (unsigned char)(i & 0x7F);
            if (isdigit(ch)) ++hits;
        }
        auto t1 = std::chrono::steady_clock::now();
        printf("  isdigit(ch)        : %6.1f ms (hits=%ld)\n",
               std::chrono::duration<double, std::milli>(t1 - t0).count(), hits);
        sink = hits;
    }
    {
        auto t0 = std::chrono::steady_clock::now();
        long hits = 0;
        for (int i = 0; i < N; ++i) {
            unsigned char ch = (unsigned char)(i & 0x7F);
            if (ch >= '0' && ch <= '9') ++hits;
        }
        auto t1 = std::chrono::steady_clock::now();
        printf("  ch>='0' && ch<='9' : %6.1f ms (hits=%ld)\n",
               std::chrono::duration<double, std::milli>(t1 - t0).count(), hits);
        sink = hits;
    }
    printf("  ※ 手写胜出：cctype 要查一张全局表（一次内存访问），手写是两条立即数比较。\n"
           "    报文校验热路径上这个差距值得用；一般代码用 cctype 可读性更好。\n");

    printf("⑤ toupper 返回 int，要转回 char\n");
    char lower = 'a';
    int up = toupper(static_cast<unsigned char>(lower));
    printf("  toupper('a') = %d → (char)='%c'\n", up, (char)up);
    return 0;
}
