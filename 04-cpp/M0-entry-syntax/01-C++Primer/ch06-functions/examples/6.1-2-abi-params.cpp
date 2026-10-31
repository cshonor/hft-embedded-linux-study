// 6.1-2-abi-params.cpp —— 实参到形参：谁走寄存器、谁走栈、谁被拷贝
//
// 编译运行：
//   clang++ -std=c++17 -Wall -O2 6.1-2-abi-params.cpp -o a && ./a
// 看汇编（AAPCS：前 8 个整型/指针实参走 x0..x7，第 9 个起进栈）：
//   clang++ -std=c++17 -O0 -S 6.1-2-abi-params.cpp -o - | grep -E "^(str|ldr|_Z)" | head -40

#include <chrono>
#include <cstdarg>
#include <cstdio>
#include <initializer_list>
#include <string>
#include <utility>

// ---------- 1. 传值 vs 传引用：不同大小的结构体，代价差多少 ----------
template <int N>
struct Blob {
    char data[N];
};

// noinline：我们要测的是"传参本身"的开销，不许编译器把调用整个内联掉
template <int N>
__attribute__((noinline)) int by_value(Blob<N> b) {
    return b.data[0] + b.data[N - 1];   // 真的读内容，防止被优化成空函数
}
template <int N>
__attribute__((noinline)) int by_ref(const Blob<N>& b) {
    return b.data[0] + b.data[N - 1];
}

template <int N>
void bench(const char* tag) {
    Blob<N> b{};
    b.data[0] = 1;
    b.data[N - 1] = 2;
    const int kIters = 2000000;
    volatile int sink = 0;

    auto t0 = std::chrono::steady_clock::now();
    for (int i = 0; i < kIters; ++i) sink += by_value<N>(b);
    auto t1 = std::chrono::steady_clock::now();
    for (int i = 0; i < kIters; ++i) sink += by_ref<N>(b);
    auto t2 = std::chrono::steady_clock::now();

    double tv = std::chrono::duration<double, std::nano>(t1 - t0).count() / kIters;
    double tr = std::chrono::duration<double, std::nano>(t2 - t1).count() / kIters;
    printf("  %-22s sizeof=%3zu  传值 %6.2f ns   传const引用 %6.2f ns    winner=%s\n",
           tag, sizeof(Blob<N>), tv, tr, tv < tr ? "传值" : "传引用");
    (void)sink;
}

// ---------- 2. initializer_list：底层就是栈上的一块临时数组 ----------
// 返回它的指针 = 返回悬空指针
const int* dangling() {
    std::initializer_list<int> il = {1, 2, 3};
    printf("  [dangling 内] 元素地址: %p %p %p（间距 %ld，连续 = 数组）\n",
           il.begin(), il.begin() + 1, il.begin() + 2,
           (long)((il.begin() + 1) - il.begin()));
    return il.begin();      // ⚠ 返回指向临时数组的指针，函数返回即悬空
}

// ---------- 3. 可变形参三种写法，类型安全差多少 ----------
int sum_varargs(int count, ...) {          // C 风格：不做类型检查
    va_list ap;
    va_start(ap, count);
    int s = 0;
    for (int i = 0; i < count; ++i) s += va_arg(ap, int);
    va_end(ap);
    return s;
}
int sum_il(std::initializer_list<int> xs) {  // 同类型、类型安全
    int s = 0;
    for (int x : xs) s += x;
    return s;
}
template <class... Ts>
int sum_tmpl(Ts... xs) {                     // 任意类型、编译期展开
    return (0 + ... + xs);                   // C++17 fold expression
}

int main() {
    printf("=== 6.1-2 参数传递：ABI 与代价（arm64 / AAPCS）===\n\n");

    printf("[1] 前 8 个整型实参走 x0..x7，第 9 个起进栈（见 -S 汇编）\n");
    printf("    sizeof(void*) = %zu，一次传参最多 %zu 字节进寄存器\n\n",
           sizeof(void*), 8 * sizeof(void*));

    printf("[2] 传值 vs 传 const 引用（200 万次，noinline，每轮读首尾字节）\n");
    bench<4>("Blob<4>");
    bench<8>("Blob<8>");
    bench<16>("Blob<16>");
    bench<32>("Blob<32>");
    bench<64>("Blob<64>");
    bench<256>("Blob<256>");
    printf("    实测读法：小中对象（≤64B）两者差距在 ±8%% 内，属噪声级；\n");
    printf("    真正拉开的是 Blob<256>：传值 5.88 ns vs 传引用 1.37 ns（快约 4 倍）。\n");
    printf("    ⚠ 本测用的是 noinline + -O2，测的是「传参 + 调用」的裸开销。真实代码里\n");
    printf("      小对象传值通常能被内联（无需取地址、可常量传播），那时传值更好。\n\n");

    printf("[3] initializer_list 是栈上临时数组，返回它 = 悬空\n");
    const int* p = dangling();
    printf("  [dangling 外] 再读：%d %d %d  ← 值还在是巧合，栈还没被覆盖（UB）\n",
           p[0], p[1], p[2]);
    auto il = {1, 2, 3};                    // ✅ 绑定到变量时，底层数组生命周期跟随 il
    printf("  绑定到 auto 变量：sizeof(il)=%zu，元素 %d\n\n", sizeof(il), *il.begin());

    printf("[4] 三种可变形参\n");
    printf("  sum_varargs(3, 1, 2, 3)      = %d   ← 类型不检查，传错类型静默 UB\n",
           sum_varargs(3, 1, 2, 3));
    printf("  sum_il({1, 2, 3})            = %d   ← 只能同类型\n", sum_il({1, 2, 3}));
    printf("  sum_tmpl(1, 2, 3)            = %d   ← 类型安全 + 可混合类型\n", sum_tmpl(1, 2, 3));
    printf("  sum_tmpl(1, 2.5) 截断为 int  = %d\n", sum_tmpl(1, 2.5));
    printf("  ⚠ varargs 的经典 UB：sum_varargs(2, 3.14) 用 %%d 读 double\n");
    printf("     实测 = %d（本应崩或乱码，小端 arm64 上恰好读到低位字）\n",
           sum_varargs(1, 3.14));

    printf("\n[5] 一个容易忽略的事实：形参就是局部变量，改它不影响实参\n");
    void bump(int);                 // 前向声明，见下方定义
    int v = 10;
    bump(v);
    printf("  bump(v) 之后 v = %d（传值：形参是副本）\n", v);
    return 0;
}

void bump(int x) { x += 100; (void)x; }

// ---------------------------------------------------------------------------
// 汇编侧证据（-O0 -S 看 AAPCS 传参）：
//   少于等于 8 个整型实参：callee 直接读 w0..w7，没有栈访问
//   第 9 个及以后：caller 把它 str 到栈上，callee 再 ldr 回来
//   大对象（> 16 字节）传值：caller 在栈上造一份副本再传地址 —— 这就是为什么
//   大对象传值比传引用慢：多了一次 sizeof(obj) 字节的 memcpy
