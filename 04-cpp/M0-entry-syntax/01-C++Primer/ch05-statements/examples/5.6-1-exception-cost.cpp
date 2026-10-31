// 5.6 异常的三笔账：正常路径零成本 / 抛出很贵 / noexcept 抛异常直接终止
//
// 编译：  clang++ -std=c++17 -O2 5.6-1-exception-cost.cpp -o exc && ./exc
// 看终止：./exc x        （noexcept 里抛异常 → std::terminate，exit=134）
// 关异常：clang++ -std=c++17 -O2 -fno-exceptions 5.6-1-exception-cost.cpp   （try/throw 直接编译错误）
#include <chrono>
#include <cstdio>
#include <stdexcept>

struct Probe {
    const char* n;
    explicit Probe(const char* s) : n(s) { printf("    + 构造 %s\n", n); }
    ~Probe() { printf("    - 析构 %s（栈展开调用）\n", n); }
};

// ① 栈展开：抛出时局部对象按构造逆序析构
static void inner() {
    Probe a("inner-a");
    Probe b("inner-b");
    throw std::runtime_error("boom");
}

// ② 正常路径：带 try/catch 与不带，是否一样快
static int __attribute__((noinline)) work(int x) { return x * 3 + 1; }

static long with_try(int n) {
    long s = 0;
    for (int i = 0; i < n; ++i) {
        try { s += work(i); }          // 从不抛出
        catch (...) { s -= 1; }
    }
    return s;
}
static long without_try(int n) {
    long s = 0;
    for (int i = 0; i < n; ++i) s += work(i);
    return s;
}

// ③ 异常路径 vs 错误码路径
static long throw_path(int n) {
    long s = 0;
    for (int i = 0; i < n; ++i) {
        try { throw std::runtime_error("x"); }
        catch (const std::exception&) { s += 1; }
    }
    return s;
}
static int __attribute__((noinline)) ec_work(int x, int& ec) {
    if (x < 0) { ec = -1; return 0; }
    ec = 0; return x * 3 + 1;
}
static long ec_path(int n) {
    long s = 0;
    for (int i = 0; i < n; ++i) {
        int ec; int r = ec_work(i, ec);
        if (ec != 0) s -= 1; else s += r;
    }
    return s;
}

template <class F>
static void bench(const char* name, F fn) {
    auto t0 = std::chrono::steady_clock::now();
    long sink = fn();
    auto t1 = std::chrono::steady_clock::now();
    double us = std::chrono::duration<double, std::micro>(t1 - t0).count();
    printf("  %-18s %10.1f μs   sink=%ld\n", name, us, sink);
}

int main(int argc, char**) {
    printf("① 栈展开时的析构顺序\n");
    try { inner(); } catch (const std::exception& e) { printf("    catch: %s\n", e.what()); }

    printf("② 正常路径（100 万次调用，从不抛出）\n");
    const int N = 1'000'000;
    bench("带 try/catch", [&] { return with_try(N); });
    bench("不带 try/catch", [&] { return without_try(N); });

    printf("③ 异常路径 vs 错误码路径（各 10 万次）\n");
    const int M = 100'000;
    bench("throw + catch", [&] { return throw_path(M); });
    bench("返回错误码", [&] { return ec_path(M); });

    if (argc > 1) {
        printf("④ noexcept 函数里抛出 →\n");
        fflush(stdout);
        auto f = []() noexcept { throw std::runtime_error("in noexcept"); };
        f();
    }
    return 0;
}
