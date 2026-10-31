// 1.6-2  "分组累加"模式的三种实现与性能对比 —— 有序归并 vs map vs unordered_map
//
// 编译：clang++ -std=c++17 -O2 -Wall 1.6-2-group-agg-perf.cpp -o agg && ./agg
//
// 对应 HFT 场景：按合约号聚合成交量 / 按 symbol 汇总成交额。
// 关键取舍：
//   · 有序归并最快，但【要求输入按 key 有序】（行情网关按 symbol 分流后才能用）
//   · unordered_map 不要求有序，代价约 2~3 倍（哈希 + 可能的 rehash）
//   · std::map 最慢（红黑树，每次比较都是字符串比较 + 指针跳转）
//
// 实测（clang 23.1.0 -O2，100 万条记录 / 1000 个不同 key，只统计聚合阶段）：
//   ① 有序归并（书店程序模式）  :  3.94 ms
//   ② std::map（乱序）          : 63.16 ms   ← 约 16 倍
//   ③ unordered_map（乱序）     : 12.50 ms   ← 约 3.2 倍
//   ②' std::map（有序）         : 45.78 ms
//   ③' unordered_map（有序）    :  6.37 ms

#include <chrono>
#include <cstdio>
#include <map>
#include <string>
#include <unordered_map>
#include <vector>

struct Rec { std::string key; unsigned qty; double rev; };

static std::vector<Rec> make(int n, int distinct, bool sorted) {
    std::vector<Rec> v;
    v.reserve(n);
    for (int i = 0; i < n; ++i) {
        int k = sorted ? (i * distinct / n)          // 有序：同 key 连续
                       : (i * 7919 % distinct);      // 乱序：跳跃
        v.push_back({"SYM-" + std::to_string(k), 1, 10.0});
    }
    return v;
}

template <class F>
static double ms(F&& f) {
    auto t0 = std::chrono::steady_clock::now();
    f();
    auto t1 = std::chrono::steady_clock::now();
    return std::chrono::duration<double, std::milli>(t1 - t0).count();
}

int main() {
    const int N = 1000000, D = 1000;
    auto sorted   = make(N, D, true);
    auto shuffled = make(N, D, false);

    volatile long long sink = 0;   // 防优化：结果必须是"可能被外部看到"的

    // ① 有序归并：书店程序的模式，O(n)、无额外内存
    auto group_scan = [&](const std::vector<Rec>& v) {
        if (v.empty()) return;
        long long total = 0; double rsum = 0;
        std::string cur = v[0].key;
        unsigned q = 0; double r = 0;
        for (const auto& e : v) {
            if (e.key == cur) { q += e.qty; r += e.rev; }
            else { total += q; rsum += r; cur = e.key; q = e.qty; r = e.rev; }
        }
        total += q; rsum += r;
        sink = total + static_cast<long long>(rsum);
    };

    // ② std::map：红黑树，O(n log k)，不要求有序
    auto map_acc = [&](const std::vector<Rec>& v) {
        std::map<std::string, std::pair<unsigned, double>> m;
        for (const auto& e : v) { auto& x = m[e.key]; x.first += e.qty; x.second += e.rev; }
        long long total = 0;
        for (auto& kv : m) total += kv.second.first;
        sink = total;
    };

    // ③ unordered_map：哈希，O(n) 均摊，不要求有序；reserve 能省掉 rehash
    auto umap_acc = [&](const std::vector<Rec>& v) {
        std::unordered_map<std::string, std::pair<unsigned, double>> m;
        m.reserve(1024);
        for (const auto& e : v) { auto& x = m[e.key]; x.first += e.qty; x.second += e.rev; }
        long long total = 0;
        for (auto& kv : m) total += kv.second.first;
        sink = total;
    };

    printf("N=%d, 不同 key=%d（只测聚合阶段，不含构造数据）\n", N, D);
    printf("  ① 有序归并（书店程序）   : %8.2f ms\n", ms([&]{ group_scan(sorted); }));
    printf("  ② std::map（乱序）       : %8.2f ms\n", ms([&]{ map_acc(shuffled); }));
    printf("  ③ unordered_map（乱序）  : %8.2f ms\n", ms([&]{ umap_acc(shuffled); }));
    printf("  ②' std::map（有序）      : %8.2f ms\n", ms([&]{ map_acc(sorted); }));
    printf("  ③' unordered_map（有序） : %8.2f ms\n", ms([&]{ umap_acc(sorted); }));
    return 0;
}
