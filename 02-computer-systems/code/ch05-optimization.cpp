/*
 * CSAPP Ch5 · 优化程序性能 — C++ 版
 *
 * 对照笔记:
 *   chapter-05/notes/section-5.6-消除不必要的内存引用.md
 *   chapter-05/notes/section-5.8-循环展开.md
 *   chapter-05/notes/section-5.9-提高并行性.md
 *
 * 编译:
 *   g++ -Wall -Wextra -std=c++17 -O2 -o ch05_opt_cpp ch05-optimization.cpp -lm
 * 运行:
 *   ./ch05_opt_cpp
 *
 * C++ 差异:
 *   - constexpr 替代 #define
 *   - std::span (C++20) 或指针+长度 传递数组视图
 *   - __restrict__ (GCC 扩展) 替代 C 的 restrict
 *   - 模板化: 同一优化策略可复用于 float/double
 *   - std::accumulate 展示标准库写法 (编译器可向量化)
 */

#include <cstdio>
#include <cstdlib>
#include <chrono>
#include <numeric>
#include <algorithm>

static constexpr int N = 1024 * 1024 * 16;  // 16M elements

// ---------- 版本 1: 原始 — 每轮读写 *dest ----------
void combine1(const double* data, int len, double* dest)
{
    *dest = 0.0;
    for (int i = 0; i < len; i++)
        *dest = *dest + data[i];
}

// ---------- 版本 2: 累加器提到寄存器 ----------
void combine2(const double* data, int len, double* dest)
{
    double acc = 0.0;
    for (int i = 0; i < len; i++)
        acc += data[i];
    *dest = acc;
}

// ---------- 版本 3: __restrict__ (GCC 扩展, C++ 无标准 restrict) ----------
void combine3(const double* __restrict__ data, int len,
              double* __restrict__ dest)
{
    double acc = 0.0;
    for (int i = 0; i < len; i++)
        acc += data[i];
    *dest = acc;
}

// ---------- 版本 4: 2x1 循环展开 ----------
void combine4(const double* __restrict__ data, int len,
              double* __restrict__ dest)
{
    double acc = 0.0;
    int limit = len - 1;
    int i;
    for (i = 0; i < limit; i += 2)
        acc = (acc + data[i]) + data[i + 1];
    for (; i < len; i++)
        acc += data[i];
    *dest = acc;
}

// ---------- 版本 5: 2x2 双累加器 (ILP) ----------
void combine5(const double* __restrict__ data, int len,
              double* __restrict__ dest)
{
    double acc0 = 0.0, acc1 = 0.0;
    int limit = len - 1;
    int i;
    for (i = 0; i < limit; i += 2) {
        acc0 += data[i];
        acc1 += data[i + 1];
    }
    for (; i < len; i++)
        acc0 += data[i];
    *dest = acc0 + acc1;
}

// ---------- 版本 6: std::accumulate (标准库, 可能被向量化) ----------
void combine6(const double* data, int len, double* dest)
{
    *dest = std::accumulate(data, data + len, 0.0);
}

// ---------- 版本 7: 模板化 — 同一策略适用 float/double ----------
template<typename T>
void combine5_tmpl(const T* __restrict__ data, int len,
                   T* __restrict__ dest)
{
    T acc0 = T{}, acc1 = T{};
    int limit = len - 1;
    int i;
    for (i = 0; i < limit; i += 2) {
        acc0 += data[i];
        acc1 += data[i + 1];
    }
    for (; i < len; i++)
        acc0 += data[i];
    *dest = acc0 + acc1;
}

// ---------- 计时 ----------
static double now_sec()
{
    using namespace std::chrono;
    return duration<double>(steady_clock::now().time_since_epoch()).count();
}

#define BENCH(fn, label) do {                          \
    double dest = 0.0, t0 = now_sec();                 \
    fn(data, N, &dest);                                \
    double t1 = now_sec();                             \
    printf("  %-32s  %8.3f ms   result=%.1f\n",        \
           label, (t1 - t0) * 1000.0, dest);           \
} while (0)

int main()
{
    double* data = static_cast<double*>(malloc(sizeof(double) * N));
    if (!data) { perror("malloc"); return 1; }

    for (int i = 0; i < N; i++)
        data[i] = static_cast<double>(i % 100) * 0.01;

    printf("=== CSAPP Ch5 · 优化对比 C++ (N=%d) ===\n\n", N);

    BENCH(combine1,            "v1 原始 (内存读写)");
    BENCH(combine2,            "v2 累加器→寄存器");
    BENCH(combine3,            "v3 +__restrict__");
    BENCH(combine4,            "v4 2x1 循环展开");
    BENCH(combine5,            "v5 2x2 双累加器");
    BENCH(combine6,            "v6 std::accumulate");
    BENCH(combine5_tmpl<double>, "v7 模板化<double>");

    printf("\nC++ 特有点:\n");
    printf("  - __restrict__ 是 GCC/Clang 扩展, 标准 C++ 无 restrict\n");
    printf("  - std::accumulate 在 -O2 下可能被自动向量化\n");
    printf("  - 模板化: combine5_tmpl<float> 可直接复用\n");
    printf("  - C++20 std::span 可替代裸指针+长度\n");

    free(data);
    return 0;
}
