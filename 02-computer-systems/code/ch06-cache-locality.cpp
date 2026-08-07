/*
 * CSAPP Ch6 · 缓存局部性 — C++ 版
 *
 * 对照笔记:
 *   chapter-06/notes/section-6.2-局部性.md
 *
 * 编译:
 *   g++ -Wall -Wextra -std=c++17 -O2 -o ch06_cache_cpp ch06-cache-locality.cpp
 * 运行:
 *   ./ch06_cache_cpp
 *
 * C++ 差异:
 *   - constexpr 替代 #define
 *   - std::vector 管理内存 (RAII)
 *   - 2D 用 1D + index 计算 (cache-friendly 且避免指针间接寻址)
 *   - 结构化绑定 / lambda 遍历
 *   - AoS/SoA 用 struct + 模板
 */

#include <cstdio>
#include <vector>
#include <chrono>
#include <cassert>

static constexpr int DIM = 4096;

// 2D matrix as 1D (row-major), 避免 double** 的指针间接寻址
class Matrix {
    std::vector<double> data_;
    int dim_;
public:
    explicit Matrix(int dim)
        : data_(static_cast<size_t>(dim) * dim), dim_(dim) {}

    double& operator()(int i, int j) { return data_[i * dim_ + j]; }
    double  operator()(int i, int j) const { return data_[i * dim_ + j]; }
    int     dim() const { return dim_; }
    const double* row(int i) const { return &data_[i * dim_]; }
};

// ---------- 行优先 ----------
double sum_row_major(const Matrix& m)
{
    double sum = 0.0;
    for (int i = 0; i < m.dim(); i++)
        for (int j = 0; j < m.dim(); j++)
            sum += m(i, j);
    return sum;
}

// ---------- 列优先 ----------
double sum_col_major(const Matrix& m)
{
    double sum = 0.0;
    for (int j = 0; j < m.dim(); j++)
        for (int i = 0; i < m.dim(); i++)
            sum += m(i, j);
    return sum;
}

// ---------- 分块 ----------
static constexpr int BLOCK = 64;
double sum_blocked(const Matrix& m)
{
    double sum = 0.0;
    int n = m.dim();
    for (int ii = 0; ii < n; ii += BLOCK)
        for (int jj = 0; jj < n; jj += BLOCK)
            for (int i = ii; i < ii + BLOCK && i < n; i++) {
                const double* row = m.row(i);
                for (int j = jj; j < jj + BLOCK && j < n; j++)
                    sum += row[j];
            }
    return sum;
}

// ---------- AoS ----------
struct OrderAoS {
    int    order_id;
    double price;
    int    qty;
    double timestamp;
};

double sum_price_AoS(const std::vector<OrderAoS>& orders)
{
    double sum = 0.0;
    for (const auto& o : orders)
        sum += o.price;
    return sum;
}

// ---------- SoA ----------
struct OrderSoA {
    std::vector<int>    order_id;
    std::vector<double> price;
    std::vector<int>    qty;
    std::vector<double> timestamp;
};

double sum_price_SoA(const OrderSoA& orders)
{
    double sum = 0.0;
    for (double p : orders.price)   // stride-1, 完美缓存利用
        sum += p;
    return sum;
}

// ---------- 计时 ----------
static double now_sec()
{
    using namespace std::chrono;
    return duration<double>(steady_clock::now().time_since_epoch()).count();
}

int main()
{
    printf("=== CSAPP Ch6 · 缓存局部性对比 C++ (%dx%d) ===\n\n", DIM, DIM);

    Matrix m(DIM);
    for (int i = 0; i < DIM; i++)
        for (int j = 0; j < DIM; j++)
            m(i, j) = static_cast<double>((i + j) % 100) * 0.01;

    double t0, t1, result;

    t0 = now_sec();
    result = sum_row_major(m);
    t1 = now_sec();
    printf("  行优先 (stride-1):   %8.3f ms   sum=%.1f\n",
           (t1 - t0) * 1000.0, result);

    t0 = now_sec();
    result = sum_col_major(m);
    t1 = now_sec();
    printf("  列优先 (stride-DIM): %8.3f ms   sum=%.1f\n",
           (t1 - t0) * 1000.0, result);

    t0 = now_sec();
    result = sum_blocked(m);
    t1 = now_sec();
    printf("  分块 (BLOCK=%d):     %8.3f ms   sum=%.1f\n",
           BLOCK, (t1 - t0) * 1000.0, result);

    // AoS vs SoA
    printf("\n=== AoS vs SoA (遍历 price 字段) ===\n\n");

    constexpr int N = 1024 * 1024;
    std::vector<OrderAoS> aos(N);
    OrderSoA soa;
    soa.order_id.resize(N);
    soa.price.resize(N);
    soa.qty.resize(N);
    soa.timestamp.resize(N);

    for (int i = 0; i < N; i++) {
        aos[i].order_id  = i;
        aos[i].price     = static_cast<double>(i % 1000) * 0.1;
        aos[i].qty       = i % 500;
        aos[i].timestamp = static_cast<double>(i);
        soa.price[i]     = aos[i].price;
    }

    t0 = now_sec();
    double r1 = sum_price_AoS(aos);
    t1 = now_sec();
    printf("  AoS (stride=%zuB):  %8.3f ms   sum=%.1f\n",
           sizeof(OrderAoS), (t1 - t0) * 1000.0, r1);

    t0 = now_sec();
    double r2 = sum_price_SoA(soa);
    t1 = now_sec();
    printf("  SoA (stride=8B):    %8.3f ms   sum=%.1f\n",
           (t1 - t0) * 1000.0, r2);

    printf("\nC++ 特有点:\n");
    printf("  - Matrix 用 1D vector + index 计算, 避免 double** 间接寻址\n");
    printf("  - range-for 遍历更安全 (无越界风险)\n");
    printf("  - RAII: vector 自动释放, 无需 free\n");
    printf("  - C++23 std::mdspan 可更优雅处理多维\n");

    return 0;
}
