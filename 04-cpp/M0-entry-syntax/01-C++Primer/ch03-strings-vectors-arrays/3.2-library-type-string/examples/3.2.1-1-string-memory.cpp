// 3.2.1 —— string 的内存实测：SSO 阈值、短串 vs 长串、拷贝 vs 移动、string_view
// 编译：clang++ -std=c++17 -O2 examples/3.2.1-1-string-memory.cpp -o strmem && ./strmem
#include <chrono>
#include <cstdio>
#include <string>
#include <string_view>
#include <utility>

int main() {
    printf("① SSO 阈值探测（libc++ arm64）\n");
    std::string s = "xy";
    printf("  sizeof(std::string) = %zu, capacity() = %zu\n", sizeof(std::string), s.capacity());
    printf("  \"xy\"   : data()=%p &s=%p → %s\n", (void*)s.data(), (void*)&s,
           s.data() == (const char*)&s ? "对象内(SSO)" : "堆上");
    std::string l(100, 'x');
    printf("  100 字符: data()=%p &s=%p → %s\n", (void*)l.data(), (void*)&l,
           l.data() == (const char*)&l ? "对象内" : "堆上");
    for (size_t n = 1; n <= 40; ++n) {
        std::string t(n, 'z');
        if (t.data() != (const char*)&t) { printf("  → 长度 >= %zu 才上堆\n", n); break; }
    }
    printf("  ※ libstdc++/MSVC 的阈值是 15，不是 22 —— 别写进可移植代码\n");

    printf("② 拷贝代价：短串 vs 长串（100 万次）\n");
    const int N = 1'000'000;
    volatile size_t sink = 0;
    {
        std::string src(100, 'x');
        auto t0 = std::chrono::steady_clock::now();
        size_t acc = 0;
        for (int i = 0; i < N; ++i) { std::string c = src; acc += c.size(); }
        auto t1 = std::chrono::steady_clock::now();
        printf("  长串(100 字符)拷贝 : %6.1f ms  (100 万次 new+delete)\n",
               std::chrono::duration<double, std::milli>(t1 - t0).count());
        sink = acc;
    }
    {
        std::string sh(20, 'y');                    // SSO 范围内
        auto t0 = std::chrono::steady_clock::now();
        size_t acc = 0;
        for (int i = 0; i < N; ++i) { std::string c = sh; acc += c.size(); }
        auto t1 = std::chrono::steady_clock::now();
        printf("  短串(20 字符)拷贝  : %6.1f ms  (0 次分配)  ← 约 600 倍\n",
               std::chrono::duration<double, std::milli>(t1 - t0).count());
        sink = acc;
    }

    printf("③ 拷贝 vs 移动 vs string_view（100 万次）\n");
    std::string src(100, 'x');
    {
        auto t0 = std::chrono::steady_clock::now();
        size_t acc = 0;
        for (int i = 0; i < N; ++i) { std::string m = std::string(src); acc += m.size(); }
        auto t1 = std::chrono::steady_clock::now();
        printf("  临时 → 移动        : %6.1f ms  (省字符拷贝，不省那次分配)\n",
               std::chrono::duration<double, std::milli>(t1 - t0).count());
        sink = acc;
    }
    {
        auto t0 = std::chrono::steady_clock::now();
        size_t acc = 0;
        for (int i = 0; i < N; ++i) { std::string_view v(src); acc += v.size(); }
        auto t1 = std::chrono::steady_clock::now();
        printf("  string_view        : %6.1f ms  (零拷贝、零分配)\n",
               std::chrono::duration<double, std::milli>(t1 - t0).count());
        sink = acc;
    }

    printf("④ sizeof 对比\n");
    printf("  sizeof(string)=%zu  sizeof(string_view)=%zu  (view = 指针+长度)\n",
           sizeof(std::string), sizeof(std::string_view));

    printf("⑤ string s = 'c'; 的报错（放开下一行注释可见）\n");
    // std::string bad = 'c';   // ❌ error: no viable conversion from 'char' to 'std::string'
    std::string ok1(1, 'c');
    std::string ok2 = "c";
    printf("  string(1,'c')=\"%s\"  string=\"c\"=\"%s\"\n", ok1.c_str(), ok2.c_str());
    return 0;
}
