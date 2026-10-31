// 6.1 参数传递：拷贝计数 / 大对象开销 / initializer_list 布局
//
// 编译运行：
//   clang++ -std=c++17 -O2 6.1-1-param-passing.cpp -o p && ./p
//
// 看传值 vs 传引用的汇编差异（需同目录的 abi-value-vs-ref.cpp）：
//   clang++ -std=c++17 -O0 -S abi-value-vs-ref.cpp -o - | grep -A4 "^__Z2f"
//   结论：fr(const int&) 与 fp(const int*) 指令逐条相同 —— 引用在 ABI 层就是指针

#include <chrono>
#include <cstdio>
#include <initializer_list>

// ---------- 1) 拷贝计数：传值 vs const T& ----------
static long g_copies = 0;

struct Counted {
    int v;
    explicit Counted(int x = 0) : v(x) {}
    Counted(const Counted& o) : v(o.v) { ++g_copies; }   // 拷贝即计数
};

__attribute__((noinline)) static void by_value(Counted x) { (void)x; }
__attribute__((noinline)) static void by_cref(const Counted& x) { (void)x; }

// ---------- 2) 大对象开销：拷贝构造带副作用，防编译器消除 ----------
static volatile char sink;

struct BigC {
    char buf[4096];
    BigC() { buf[0] = 1; }
    BigC(const BigC& o) {
        for (size_t i = 0; i < sizeof buf; ++i) buf[i] = o.buf[i];
        sink ^= buf[0];               // 副作用：让拷贝无法被优化掉
    }
};

__attribute__((noinline)) static void big_value(BigC b) { sink ^= b.buf[0]; }
__attribute__((noinline)) static void big_cref(const BigC& b) { sink ^= b.buf[0]; }

int main() {
    using clk = std::chrono::steady_clock;

    printf("===== 1) 拷贝次数（同一个对象调用 1000 次）=====\n");
    Counted c(7);
    g_copies = 0;
    for (int i = 0; i < 1000; ++i) by_value(c);
    printf("  by_value(Counted)        拷贝构造 %ld 次\n", g_copies);
    g_copies = 0;
    for (int i = 0; i < 1000; ++i) by_cref(c);
    printf("  by_cref(const Counted&)  拷贝构造 %ld 次\n", g_copies);
    printf("  → 传值每次都要构造形参副本；const T& 只是绑定，零拷贝\n");

    printf("\n===== 2) 4KB 大对象传参 20 万次=====\n");
    const int N = 200000;
    BigC big{};
    auto t0 = clk::now();
    for (int i = 0; i < N; ++i) big_value(big);
    auto t1 = clk::now();
    auto t2 = clk::now();
    for (int i = 0; i < N; ++i) big_cref(big);
    auto t3 = clk::now();
    double mv = std::chrono::duration<double, std::milli>(t1 - t0).count();
    double mr = std::chrono::duration<double, std::milli>(t3 - t2).count();
    printf("  big_value(BigC 4KB) : %6.2f ms  (单次 %.1f ns)\n", mv, mv * 1e6 / N);
    printf("  big_cref (BigC 4KB) : %6.2f ms  (单次 %.1f ns)\n", mr, mr * 1e6 / N);
    printf("  → 若把 BigC 换成 trivially copyable 且只读一个字节，实测两者都 ~0.57ms：\n");
    printf("    编译器把拷贝优化没了。测真实差距必须让拷贝构造带副作用。\n");

    printf("\n===== 3) initializer_list 是栈上临时数组的视图 =====\n");
    std::initializer_list<int> il = {1, 2, 3, 4, 5};
    printf("  sizeof(initializer_list<int>) = %zu   (首地址 + 长度，两个指针)\n", sizeof(il));
    printf("  il.begin() = %p   ← 栈地址，不是堆\n", (void*)il.begin());
    printf("  → 不能返回它、不能存进成员：函数返回后底层数组即失效\n");

    printf("\n===== 4) 非 const 引用拒绝右值 =====\n");
    printf("  void f(int&);      f(42);  → error: no matching function for call to 'f'\n");
    printf("  void f(const int&); f(42); → 通过（绑定到临时对象）\n");

    return 0;
}
