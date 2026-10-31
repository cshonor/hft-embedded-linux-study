// 5.3 switch 的两种代码生成 + 分支预测到底值多少钱
//
// 编译：clang++ -std=c++17 -O2 5.3-1-switch-and-branch.cpp -o cond && ./cond
// 看汇编：clang++ -std=c++17 -O2 -S 5.3-1-switch-and-branch.cpp -o - | grep -E "switch.table|cmp|b\.eq|ret"
#include <algorithm>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <vector>

// ① 紧凑 case（0..5 连续，返回值刻意不连续）→ 编译器查跳转表
int dense(int k) {
    switch (k) {
        case 0: return 100;  case 1: return -7;  case 2: return 42;
        case 3: return 3;    case 4: return 999; case 5: return 8;
        default: return -1;
    }
}

// ② 稀疏 case（数值跨度大）→ 退化成 if-else 链
int sparse(int k) {
    switch (k) {
        case 1:        return 10;
        case 1000:     return 11;
        case 100000:   return 12;
        case 10000000: return 13;
        default:       return -1;
    }
}

// ③ 分支预测：同一段 if，喂随机数据 vs 已排序数据
static long sum_if(const std::vector<int>& v) {
    long s = 0;
    for (int x : v)
        if (x >= 128) s += x * 7 + 3;
        else          s -= x * 3 + 1;
    return s;
}

int main() {
    printf("① 紧凑 switch dense(3)    = %d\n", dense(3));
    printf("② 稀疏 switch sparse(1000) = %d\n", sparse(1000));

    printf("③ 分支预测（100 万 int × 5 轮，-O2）\n");
    const int N = 1'000'000;
    std::vector<int> data(N);
    for (int i = 0; i < N; ++i) data[i] = rand() % 256;
    auto sorted = data;
    std::sort(sorted.begin(), sorted.end());

    auto bench = [](const char* name, const std::vector<int>& v) {
        auto t0 = std::chrono::steady_clock::now();
        long sink = 0;
        for (int r = 0; r < 5; ++r) sink += sum_if(v);
        auto t1 = std::chrono::steady_clock::now();
        double ms = std::chrono::duration<double, std::milli>(t1 - t0).count() / 5;
        printf("  %-12s %8.3f ms/轮   sink=%ld\n", name, ms, sink);
        return ms;
    };
    double t_rand = bench("随机数据", data);
    double t_sort = bench("已排序数据", sorted);
    printf("  → 排序后快 %.2f 倍（不是传说中的 3~6 倍，见笔记说明）\n", t_rand / t_sort);
    return 0;
}
