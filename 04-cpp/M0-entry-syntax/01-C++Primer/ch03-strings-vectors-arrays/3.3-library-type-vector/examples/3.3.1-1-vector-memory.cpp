// 3.3.1 / 3.3.2 —— vector 与 string 的内存实测：SSO、扩容序列、reserve、圆/花括号
// 编译：clang++ -std=c++17 -O2 examples/3.3.1-1-vector-memory.cpp -o mem && ./mem
#include <chrono>
#include <cstdio>
#include <string>
#include <vector>

int main() {
    printf("① string 的 SSO（短字符串优化）\n");
    std::string s = "xy";
    printf("  sizeof(std::string) = %zu\n", sizeof(std::string));
    printf("  \"xy\"     : data()=%p  &s=%p  → %s\n",
           (void*)s.data(), (void*)&s, s.data() == (const char*)&s ? "对象内(SSO, 0 次分配)" : "堆上");
    printf("  capacity = %zu\n", s.capacity());
    std::string l(100, 'x');
    printf("  100 字符 : data()=%p  &s=%p  → %s\n",
           (void*)l.data(), (void*)&l, l.data() == (const char*)&l ? "对象内" : "堆上(1 次分配)");
    for (size_t n = 1; n <= 40; ++n) {
        std::string t(n, 'z');
        if (t.data() != (const char*)&t) { printf("  SSO 阈值：长度 >= %zu 才上堆\n", n); break; }
    }

    printf("② vector 扩容序列（libc++ 严格 2 倍）\n");
    std::vector<int> v;
    size_t prev = v.capacity();
    printf("  初始 capacity=%zu\n", prev);
    for (int i = 0; i < 40; ++i) {
        v.push_back(i);
        if (v.capacity() != prev) {
            printf("  push #%2d → %zu → %zu  (%.2fx)\n",
                   i + 1, prev, v.capacity(), (double)v.capacity() / (double)prev);
            prev = v.capacity();
        }
    }
    printf("  ※ libstdc++/libc++ 是 2.0x；MSVC 是 1.5x。都保证摊还 O(1)。\n");

    printf("③ reserve 的效果（1000 万次 push_back）\n");
    const int N = 10'000'000;
    {
        auto t0 = std::chrono::steady_clock::now();
        std::vector<int> a;
        for (int i = 0; i < N; ++i) a.push_back(i);
        auto t1 = std::chrono::steady_clock::now();
        printf("  无 reserve : %6.1f ms (final capacity=%zu)\n",
               std::chrono::duration<double, std::milli>(t1 - t0).count(), a.capacity());
    }
    {
        auto t0 = std::chrono::steady_clock::now();
        std::vector<int> b; b.reserve(N);
        for (int i = 0; i < N; ++i) b.push_back(i);
        auto t1 = std::chrono::steady_clock::now();
        printf("  有 reserve : %6.1f ms\n",
               std::chrono::duration<double, std::milli>(t1 - t0).count());
    }
    printf("  ※ 差距来自 24 次扩容搬移 + 最终 capacity 16777216 比 1000 万多 67%%（多占 256 MB 中的一部分）\n");

    printf("④ 圆括号 vs 花括号\n");
    std::vector<int> p1(10), b1{10}, b2{1, 2, 3};
    printf("  vector<int> v(10)    → size=%zu, v[0]=%d   (10 个 0)\n", p1.size(), p1[0]);
    printf("  vector<int> v{10}    → size=%zu, v[0]=%d    (1 个元素 10)\n", b1.size(), b1[0]);
    printf("  vector<int> v{1,2,3} → size=%zu\n", b2.size());
    std::vector<std::string> ps(3), bs{"a", "b"};
    printf("  vector<string> v(3)  → size=%zu, v[0] 空串? %d\n", ps.size(), (int)ps[0].empty());
    printf("  vector<string> v{\"a\",\"b\"} → size=%zu\n", bs.size());
    // std::vector<std::string> bad{"a", 3};  // ❌ string 不能从 int 构造

    printf("⑤ 元素地址连续（缓存友好）\n");
    std::vector<int> c; c.reserve(4);
    for (int i = 0; i < 4; ++i) c.push_back(i);
    printf("  &c[0]=%p  &c[1]=%p  相差 %ld 字节\n",
           (void*)&c[0], (void*)&c[1], (long)((char*)&c[1] - (char*)&c[0]));
    return 0;
}
