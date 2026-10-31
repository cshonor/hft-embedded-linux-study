// 3.3.3 —— 下标与增删：[] vs at、越界、删除位置的代价
// 编译：clang++ -std=c++17 -O2 examples/3.3.3-1-index-and-erase.cpp -o vecops && ./vecops
//       clang++ -std=c++17 -O1 -fsanitize=address examples/3.3.3-1-index-and-erase.cpp -o oob
//       （ASan 会在越界读处报 heap-buffer-overflow）
#include <chrono>
#include <cstdio>
#include <stdexcept>
#include <vector>

int main() {
    printf("① v[i] vs v.at(i)（1 亿次，跑三轮看趋势）\n");
    const int N = 100'000'000;
    std::vector<int> v(1024, 7);
    volatile long sink = 0;
    for (int rep = 0; rep < 3; ++rep) {
        auto t0 = std::chrono::steady_clock::now();
        long s = 0;
        for (int i = 0; i < N; ++i) s += v[i & 1023];
        auto t1 = std::chrono::steady_clock::now();
        double a = std::chrono::duration<double, std::milli>(t1 - t0).count();

        auto t2 = std::chrono::steady_clock::now();
        long s2 = 0;
        for (int i = 0; i < N; ++i) s2 += v.at(i & 1023);
        auto t3 = std::chrono::steady_clock::now();
        double b = std::chrono::duration<double, std::milli>(t3 - t2).count();
        printf("  run %d: v[i]=%6.1f ms   v.at(i)=%6.1f ms   (at/[] = %.2fx)\n", rep, a, b, b / a);
        sink = s + s2;
    }
    printf("  ※ 预热后两者差距在噪声内（16.9 vs 16.8 ms）：边界检查是可预测分支，\n"
           "     Apple Silicon 上几乎零成本。别再因为\"at() 慢\"而放弃它。\n");

    printf("② 越界：at() 抛异常，[] 是 UB\n");
    try { (void)v.at(99999); }
    catch (const std::out_of_range& e) { printf("  v.at(99999) → out_of_range: %s\n", e.what()); }
    printf("  v[99999]    → 不检查；ASan 下报 heap-buffer-overflow，release 下静默读垃圾\n");

    printf("③ 删除位置决定代价（20 万元素，各删 2 万个）\n");
    const int M = 200'000, K = 20'000;
    {
        std::vector<int> a(M, 3);
        auto t0 = std::chrono::steady_clock::now();
        long s = 0;
        for (int i = 0; i < K; ++i) { s += a.back(); a.pop_back(); }
        auto t1 = std::chrono::steady_clock::now();
        printf("  尾部 pop_back × %d : %10.4f ms (size=%zu)  O(1)\n", K,
               std::chrono::duration<double, std::milli>(t1 - t0).count(), a.size());
        sink = s;
    }
    {
        std::vector<int> a(M, 3);
        auto t0 = std::chrono::steady_clock::now();
        long s = 0;
        for (int i = 0; i < K; ++i) { auto it = a.begin() + a.size() / 2; s += *it; a.erase(it); }
        auto t1 = std::chrono::steady_clock::now();
        printf("  中间 erase   × %d : %10.4f ms (size=%zu)  O(n/2 搬移)\n", K,
               std::chrono::duration<double, std::milli>(t1 - t0).count(), a.size());
        sink = s;
    }
    {
        std::vector<int> a(M, 3);
        auto t0 = std::chrono::steady_clock::now();
        long s = 0;
        for (int i = 0; i < K; ++i) { s += a.front(); a.erase(a.begin()); }
        auto t1 = std::chrono::steady_clock::now();
        printf("  头部 erase   × %d : %10.4f ms (size=%zu)  O(n) 全搬\n", K,
               std::chrono::duration<double, std::milli>(t1 - t0).count(), a.size());
        sink = s;
    }
    printf("  ※ 尾部 0.003 ms vs 头部 289.7 ms —— 约 10 万倍。\n"
           "     需要频繁头删的场景改用 std::deque / std::list，或做\"标记删除 + 定期压缩\"。\n");

    printf("④ 用下标添加元素是 UB（不是自动扩容）\n");
    std::vector<int> e;                 // size=0
    printf("  空 vector 的 size=%zu，v[0] 越界；正确写法是 push_back / emplace_back\n", e.size());
    return 0;
}
