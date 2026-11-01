// 4.3-1 关系与逻辑运算符：短路、性能、bool 的几个数字
//
// 编译：
//   clang++ -std=c++17 -O2 examples/4.3-1-logic-assign-cond.cpp -o logic && ./logic
//
// 要点：&& || 有短路 + 有求值顺序保证；& | 没有短路也没有顺序保证。

// [2] 的 a<b<c 和 [5] 的 if(flag = 1) 是「故意写错」来演示，clang 会把前者直接判为
// error（-Wparentheses 的 chained comparison 分支默认就是 error）。这里压掉才能跑，
// 但请记住：真实代码里它根本过不了编译。
#pragma clang diagnostic ignored "-Wparentheses"

#include <chrono>
#include <cstdint>
#include <cstdio>
#include <string>

// 不可内联：模拟「昂贵」的判断（查表 / 字符串比较 / 跨 TU 调用都属这一类）
__attribute__((noinline)) static bool expensive(int i) { return (i % 1000) == 0; }

static int data[1024];            // 全为 0，但编译器看不出来（非 const、非 static 局部）
static int calls = 0;
static bool expensive() { ++calls; return true; }
static bool cheap() { ++calls; return false; }

int main(int argc, char**) {
    printf("=== 4.3-1 关系与逻辑运算符实测 ===\n\n");

    // ---------- [1] 短路的证据：数一数右侧被调了几次 ----------
    printf("[1] 短路：右侧到底被调用了吗\n");
    calls = 0;
    bool r1 = cheap() && expensive();   // 左假 -> 右不求值
    printf("    cheap() && expensive()  -> 结果 %d，共调用 %d 次（右例被跳过）\n", r1, calls);

    calls = 0;
    bool r2 = expensive() || cheap();   // 左真 -> 右不求值
    printf("    expensive() || cheap()  -> 结果 %d，共调用 %d 次（右例被跳过）\n", r2, calls);

    calls = 0;
    int r3 = cheap() | (expensive() ? 2 : 0);       // 位或：两侧都求值
    printf("    cheap() | expensive()?  -> 结果 %d，共调用 %d 次（位运算不短路！）\n\n", r3, calls);

    // ---------- [2] a < b < c 不是「介于两者之间」 ----------
    printf("[2] a < b < c 的真实含义\n");
    int a = 3, b = 10, c = 5;
    printf("    a=%d b=%d c=%d\n", a, b, c);
    printf("    a < b < c  = %d   // 等价于 (a<b) < c，即 1 < 5 -> 恒真，与 b 无关\n", (a < b < c) ? 1 : 0);
    printf("    ⚠ 不压 -Wparentheses 时这行是 error：chained comparison ... -Wparentheses\n");
    printf("    正确写法 (a<b && b<c) = %d\n\n", (a < b && b < c) ? 1 : 0);

    // ---------- [3] 短路值 5 倍性能：把便宜的判断放前面 ----------
    printf("[3] 短路带来的性能差（2e7 次）\n");
    // 设计：左侧 i < 0 几乎恒假，右侧 i % 1000 == 0 是昂贵的取模。
    // 短路生效时右侧一次都不跑；把顺序反过来，右侧每次都要跑。
    const int N = 20000000;
    volatile int sink = 0;
    int threshold = argc - 1000;   // 运行时才知道（= -999），编译器无法常量折叠

    auto t0 = std::chrono::steady_clock::now();
    for (int i = 0; i < N; ++i) {
        if (i < threshold && expensive(i)) ++sink;   // 左侧恒假 -> expensive 从不调用
    }
    auto t1 = std::chrono::steady_clock::now();

    auto t2 = std::chrono::steady_clock::now();
    for (int i = 0; i < N; ++i) {
        if (expensive(i) && i < threshold) ++sink;   // expensive 每次都调用
    }
    auto t3 = std::chrono::steady_clock::now();

    double cheap_first  = std::chrono::duration<double, std::milli>(t1 - t0).count();
    double exp_first    = std::chrono::duration<double, std::milli>(t3 - t2).count();
    printf("    便宜判断在前（短路掉右侧）：%8.2f ms\n", cheap_first);
    printf("    昂贵判断在前（每次都算）：  %8.2f ms\n", exp_first);
    printf("    比值 = %.2fx —— 把「大概率为假的便宜条件」放最前面\n\n",
           cheap_first > 0 ? exp_first / cheap_first : 0.0);

    // ---------- [4] && 不一定生成跳转指令 ----------
    printf("[4] 逻辑运算符的结果类型是 bool，不是 int\n");
    printf("    sizeof(bool)          = %zu\n", sizeof(bool));
    printf("    sizeof(true && false) = %zu\n", sizeof(true && false));
    printf("    (3 && 4) = %d  —— 任何非零都压成 1，不会得到 4\n", (3 && 4));
    printf("    汇编层面：return a && b; 在 ARM64 常被编译成 ccmp/cset，无跳转\n\n");

    // ---------- [5] == 写成 = 的坑：编译器的态度 ----------
    printf("[5] == 写成 = 会怎样\n");
    int flag = 0;
    if (flag = 1) {                       // 合法但语义全变：赋值 1，恒为真
        printf("    if (flag = 1) 分支进入了，flag 现在是 %d\n", flag);
    }
    printf("    clang 会警告 -Wparentheses；写 if (1 == flag) 或加 -Werror 可拦住\n\n");

    // ---------- [6] 关系运算符的结果是 bool，链式比较失效 ----------
    printf("[6] 混合比较：有符号 vs 无符号\n");
    int si = -1;
    unsigned ui = 1;
    printf("    int si = -1;  unsigned ui = 1;\n");
    printf("    si < ui            = %d   // ❗-1 被提升成 unsigned = 4294967295，所以「不小于」\n", (si < ui) ? 1 : 0);
    printf("    si < (int)ui       = %d   // 显式转换后才符合直觉\n", (si < (int)ui) ? 1 : 0);
    printf("    (unsigned)si       = %u\n", (unsigned)si);
    printf("    这是 -Wsign-compare 的典型场景；循环里写 for (unsigned i = 0; i < v.size()-1; ++i)\n");
    printf("    在 v 为空时 size()-1 下溢成巨大值，直接越界\n\n");

    // ---------- [7] 短路还是位运算：语义对比表 ----------
    printf("[7] 语义对比\n");
    printf("    ┌──────────┬────────┬────────────┬──────────────┐\n");
    printf("    │ 运算符   │ 短路   │ 求值顺序   │ 结果类型     │\n");
    printf("    ├──────────┼────────┼────────────┼──────────────┤\n");
    printf("    │ && / ||  │ 有     │ 左先于右   │ bool         │\n");
    printf("    │ &  / |   │ 无     │ 未指定     │ 整型（按位） │\n");
    printf("    └──────────┴────────┴────────────┴──────────────┘\n");
    printf("    判空指针再解引用必须写 p && p->x，写成 p & (p->x) 会崩\n");

    return 0;
}
