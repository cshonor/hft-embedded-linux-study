// 3.2.3 —— 拼接的分配次数实测（重载全局 operator new/delete 计数）
// 编译：clang++ -std=c++17 -O2 examples/3.2.3-1-concat-alloc.cpp -o concat && ./concat
#include <cstdio>
#include <cstdlib>
#include <new>
#include <string>

static long g_new = 0, g_del = 0;
static bool g_on = false;
void* operator new(size_t n) {
    if (g_on) ++g_new;
    void* p = malloc(n ? n : 1);
    if (!p) throw std::bad_alloc();
    return p;
}
void operator delete(void* p) noexcept { if (g_on && p) ++g_del; free(p); }
void operator delete(void* p, size_t) noexcept { if (g_on && p) ++g_del; free(p); }

#define MEASURE(label, expr) do {                        \
    g_new = g_del = 0; g_on = true;                      \
    { expr; }                                             \
    g_on = false;                                         \
    printf("  %-32s new=%ld  delete=%ld\n", label, g_new, g_del); \
} while (0)

int main() {
    std::string s1 = "IF2406", s2 = "20260630";     // 短串：都在 SSO 内
    printf("① 短串（≤ 22 字节，SSO）：全是 0 次分配\n");
    MEASURE("string c = s1;",              std::string c = s1;);
    MEASURE("string c = s1 + s2;",         std::string c = s1 + s2;);
    MEASURE("string c = s1 + \" \" + s2;",   std::string c = s1 + " " + s2;);
    MEASURE("reserve + append",            std::string c; c.reserve(16); c.append(s1).append(" ").append(s2););

    std::string a(40, 'x'), b(40, 'y');              // 长串：超过 SSO 阈值
    printf("② 长串（40+40 字符）：差距才显现\n");
    MEASURE("string c = a + b;",           std::string c = a + b;);
    MEASURE("string c = a + \" \" + b;",     std::string c = a + " " + b;);
    MEASURE("c = a; c += \" \"; c += b;",    std::string c = a; c += " "; c += b;);
    MEASURE("reserve(81) + append ×3",     std::string c; c.reserve(81); c.append(a).append(" ").append(b););

    printf("③ 循环追加 20 万次：reserve 的真正价值在这里\n");
    {
        g_new = g_del = 0; g_on = true;
        std::string s; for (int i = 0; i < 200000; ++i) s += "a";
        g_on = false;
        printf("  无 reserve: new=%ld delete=%ld (最终 capacity=%zu)\n", g_new, g_del, s.capacity());
    }
    {
        g_new = g_del = 0; g_on = true;
        std::string s; s.reserve(200000);
        for (int i = 0; i < 200000; ++i) s += "a";
        g_on = false;
        printf("  有 reserve: new=%ld delete=%ld\n", g_new, g_del);
    }
    return 0;
}
