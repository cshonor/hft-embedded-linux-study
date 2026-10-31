// 4.3 / 4.4 / 4.7：短路求值、赋值、条件运算符的实测
//
// 编译：clang++ -std=c++17 -O2 4.3-1-logic-assign-cond.cpp -o logic && ./logic
#include <chrono>
#include <cstdio>
#include <string>

// ---------- ① 短路：右侧到底执行了没有 ----------
static int calls = 0;
static bool left_false() { ++calls; return false; }
static bool left_true()  { ++calls; return true; }
static bool right_side() { ++calls; return true; }

// ---------- ③ 短路的性能价值：便宜条件挡住昂贵检查 ----------
static volatile long g_sink = 0;
__attribute__((noinline)) static bool expensive_check(int x) {
    long s = 0;
    for (int i = 0; i < 200; ++i) s += x * i;
    g_sink += s;                 // 落到 volatile 上，防止被整体优化掉
    return s > 0;
}
static volatile int g_cheap_false = 0;

// ---------- ⑤ string: += vs = ... + ... ----------
static void bench_string() {
    const int N = 200'000;
    auto t0 = std::chrono::steady_clock::now();
    std::string s1;
    for (int i = 0; i < N; ++i) s1 = s1 + "a";      // 每次产生临时 + O(n) 拷贝
    auto t1 = std::chrono::steady_clock::now();
    double ms1 = std::chrono::duration<double, std::milli>(t1 - t0).count();

    std::string s2;
    s2.reserve(N + 1);
    for (int i = 0; i < N; ++i) s2 += "a";          // 原地追加
    auto t2 = std::chrono::steady_clock::now();
    double ms2 = std::chrono::duration<double, std::milli>(t2 - t1).count();

    printf("  s = s + \"a\"  : %8.2f ms  (len=%zu)\n", ms1, s1.size());
    printf("  s += \"a\"     : %8.2f ms  (len=%zu)  → 快 %.1f 倍\n",
           ms2, s2.size(), ms1 / ms2);
}

int main() {
    printf("① 短路：右侧是否被执行（calls 计数）\n");
    calls = 0; bool r1 = left_false() && right_side();
    printf("  false && right : r=%d, calls=%d  → 右侧没调用\n", r1, calls);
    calls = 0; bool r2 = left_true()  || right_side();
    printf("  true  || right : r=%d, calls=%d  → 右侧没调用\n", r2, calls);
    calls = 0; bool r3 = left_false() &  right_side();
    printf("  false &  right : r=%d, calls=%d  → 位运算不短路，右侧照调\n", r3, calls);

    printf("② a < b < c 的坑（clang 对原式直接报 error，这里拆开写）\n");
    int a = 1, b = 2, c = 0;
    bool chained = (a < b) < c;
    printf("  a=1 b=2 c=0: (a<b)<c=%d，而 (a<b && b<c)=%d\n", chained, (a < b && b < c));
    int a2 = 5, b2 = 10, c2 = 3;
    printf("  a=5 b=10 c=3: (a<b)<c=%d ← 数学上 5 不在 (10,3) 之间却是真\n", (a2 < b2) < c2);

    printf("③ 短路的性能价值（1000 万次：便宜条件挡住昂贵检查）\n");
    {
        auto t0 = std::chrono::steady_clock::now();
        long hit = 0;
        for (int i = 0; i < 10'000'000; ++i)
            if (g_cheap_false && expensive_check(i)) ++hit;   // 短路 → 右侧从不执行
        auto t1 = std::chrono::steady_clock::now();
        printf("  便宜条件在前：%.1f ms (hit=%ld)\n",
               std::chrono::duration<double, std::milli>(t1 - t0).count(), hit);

        auto t2 = std::chrono::steady_clock::now();
        long hit2 = 0;
        for (int i = 0; i < 10'000'000; ++i)
            if (expensive_check(i) && g_cheap_false) ++hit2;  // 昂贵检查在前 → 每次都跑
        auto t3 = std::chrono::steady_clock::now();
        printf("  昂贵条件在前：%.1f ms (hit=%ld)\n",
               std::chrono::duration<double, std::milli>(t3 - t2).count(), hit2);
    }

    printf("④ 赋值\n");
    int a3, b3, c3;
    a3 = b3 = c3 = 0;
    printf("  a = b = c = 0 → %d（右结合）\n", a3 + b3 + c3);
    int x = 0;
    (x = 5) = 10;      // C++ 合法：赋值表达式是左值（C 里报 "expression is not assignable"）
    printf("  (x = 5) = 10 → x=%d（C++ 允许，C 不允许）\n", x);

    printf("⑤ string 的 += vs = ... + ...（20 万次，-O2）\n");
    bench_string();

    printf("⑥ 条件运算符的结果类型（公共类型）\n");
    auto v1 = true ? 1 : 2;
    auto v2 = true ? 1 : 2.0;
    auto v3 = true ? 'a' : 65;
    printf("  true ? 1 : 2    → sizeof=%zu\n", sizeof(v1));
    printf("  true ? 1 : 2.0  → sizeof=%zu（提升成 double）\n", sizeof(v2));
    printf("  true ? 'a' : 65 → sizeof=%zu（提升成 int）\n", sizeof(v3));

    int p = 1, q = 2;
    (p > 0 ? p : q) = 99;              // C++ 合法：条件表达式可以是左值
    printf("  (p>0 ? p : q) = 99 → p=%d q=%d（C++ 允许，C 不允许）\n", p, q);
    printf("  sizeof(bool)=%zu, true+true=%d\n", sizeof(bool), true + true);
    return 0;
}
