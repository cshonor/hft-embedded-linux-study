// 6.5 函数指针 —— 回调的三种写法，代价差多少？（实测）
//
// 编译运行：
//   clang++ -std=c++17 -O2 6.5-2-dispatch-cost.cpp -o /tmp/b && /tmp/b
//   clang++ -std=c++17 -O0 -S 6.5-2-dispatch-cost.cpp -o - | grep -E "bl|blr"   # 看调用指令
//
// 实测结论（Apple clang 23.1.0, arm64, -O2，1000 万次调用）：
//   模板内联 lambda   ≈ 0.32 ns/次   ← 被内联，循环里连函数调用都没有
//   裸函数指针        ≈ 2.0  ns/次   ← 一次间接跳转 blr，无法内联
//   std::function     ≈ 2.0  ns/次   ← 类型擦除，也是一次间接跳转
//   ⇒ 真正的分界线是「能不能内联」（约 6~10 倍），不是「裸指针还是 std::function」
//
// 另一个反直觉结论：函数指针跳转表 vs if-else 链，实测耗时基本相同（0.78 vs 0.75 ns）。
// 现代 CPU 的分支预测器能预测固定模式的 if-else，所以函数指针的价值是**可扩展性**，不是性能。

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <functional>
#include <vector>

using clk = std::chrono::steady_clock;
static volatile long vsink;        // 防优化：让调用真的发生

// ---------- ① 三种回调写法 ----------
static long add1(long x) { vsink ^= x; return x + 1; }

__attribute__((noinline)) static long via_fptr(long (*f)(long), int n) {
    long s = 0; for (int i = 0; i < n; ++i) s += f(i); return s;   // 间接调用
}
template <class F>
__attribute__((noinline)) static long via_tmpl(F f, int n) {
    long s = 0; for (int i = 0; i < n; ++i) s += f(i); return s;   // 编译期已知 → 可内联
}
__attribute__((noinline)) static long via_stdfunc(const std::function<long(long)>& f, int n) {
    long s = 0; for (int i = 0; i < n; ++i) s += f(i); return s;
}

static void three_ways() {
    const int N = 10000000, R = 3;
    // volatile 指针：挡住 clang 的「间接调用提升」（否则会把 f() 推测成 add1 并内联，测出 0 ns）
    static long (*volatile vfp)(long) = add1;

    auto bench = [&](const char* name, auto f) {
        auto t0 = clk::now(); long s = 0;
        for (int r = 0; r < R; ++r) s += f();
        auto t1 = clk::now();
        vsink ^= s;
        double ms = std::chrono::duration<double, std::milli>(t1 - t0).count() / R;
        printf("  %-24s %6.2f ms/轮 = %5.2f ns/次调用\n", name, ms, ms * 1e6 / N);
    };

    printf("===== ① 三种回调写法（%d 万次 × %d 轮）=====\n", N / 10000, R);
    bench("模板 lambda（可内联）", [&]{ return via_tmpl([](long x){ vsink ^= x; return x + 1; }, N); });
    bench("裸函数指针",            [&]{ return via_fptr(vfp, N); });
    bench("std::function",         [&]{ return via_stdfunc([](long x){ vsink ^= x; return x + 1; }, N); });

    printf("  sizeof(std::function<long(long)>) = %zu  ← 类型擦除要存 3 个指针\n",
           sizeof(std::function<long(long)>));
    printf("  sizeof(long(*)(long))             = %zu\n", sizeof(long(*)(long)));
}

// ---------- ② 跳转表 vs if-else ----------
static long op0(long x) { return x + 0; }
static long op1(long x) { return x + 1; }
static long op2(long x) { return x + 2; }
static long op3(long x) { return x + 3; }
static long (*const kTable[4])(long) = {op0, op1, op2, op3};

__attribute__((noinline)) static long via_table(int k, int n) {
    long s = 0; for (int i = 0; i < n; ++i) s += kTable[k & 3](i); return s;
}
__attribute__((noinline)) static long via_ifelse(int k, int n) {
    long s = 0;
    for (int i = 0; i < n; ++i) {
        if      (k == 0) s += op0(i);
        else if (k == 1) s += op1(i);
        else if (k == 2) s += op2(i);
        else             s += op3(i);
    }
    return s;
}

static void table_vs_ifelse() {
    const int N = 10000000, R = 3;
    auto bench = [&](const char* name, auto f) {
        auto t0 = clk::now(); long s = 0;
        for (int r = 0; r < R; ++r) s += f(r & 3);
        auto t1 = clk::now();
        vsink ^= s;
        double ms = std::chrono::duration<double, std::milli>(t1 - t0).count() / R;
        printf("  %-24s %6.2f ms/轮 = %4.2f ns/次\n", name, ms, ms * 1e6 / N);
    };
    printf("\n===== ② 跳转表 vs if-else（分支目标固定、可预测）=====\n");
    bench("函数指针跳转表", [&](int k){ return via_table(k, N); });
    bench("if-else 链",     [&](int k){ return via_ifelse(k, N); });
    printf("  ⇒ 基本持平。函数指针省的是代码行数，不是 CPU 时间\n");
}

// ---------- ③ qsort vs std::sort：回调不可内联的代价 ----------
static int cmp_int(const void* a, const void* b) {
    int x = *(const int*)a, y = *(const int*)b;
    return (x > y) - (x < y);
}

static void sort_cmp() {
    const int N = 100000, R = 20;
    std::vector<int> src(N);
    for (int i = 0; i < N; ++i) src[i] = rand();
    std::vector<int> a = src, b = src;

    auto t0 = clk::now();
    for (int r = 0; r < R; ++r) { a = src; qsort(a.data(), N, sizeof(int), cmp_int); }
    auto t1 = clk::now();
    auto t2 = clk::now();
    for (int r = 0; r < R; ++r) { b = src; std::sort(b.begin(), b.end()); }
    auto t3 = clk::now();
    double mq = std::chrono::duration<double, std::milli>(t1 - t0).count() / R;
    double ms = std::chrono::duration<double, std::milli>(t3 - t2).count() / R;
    vsink ^= a[0] + b[0];

    printf("\n===== ③ qsort vs std::sort（%d 个 int × %d 轮）=====\n", N, R);
    printf("  qsort    (比较器是函数指针，无法内联): %6.2f ms/轮\n", mq);
    printf("  std::sort(比较器是模板参数，可内联)  : %6.2f ms/轮\n", ms);
    printf("  比值: %.2f x\n", mq / ms);
    printf("  ⇒ 这是「回调 vs 内联」差距最直观的例子\n");
}

int main() {
    three_ways();
    table_vs_ifelse();
    sort_cmp();
    return 0;
}
