/*
 * Hennessy Ch2/Ch5 · 伪共享 (False Sharing) — C++ 版
 *
 * 对照笔记:
 *   03/chapter-02/notes/section-2.3-缓存性能十项高级优化.md
 *   03/chapter-05/notes/section-5.3-性能分析与伪共享.md
 *
 * 编译:
 *   g++ -Wall -Wextra -std=c++17 -O2 -o ch02_false_cpp ch02-false-sharing.cpp -lpthread
 * 运行:
 *   ./ch02_false_cpp
 *
 * C++ 差异:
 *   - alignas(64) 替代 __attribute__((aligned(64)))
 *   - std::atomic<uint64_t> 替代 _Atomic uint64_t
 *   - std::thread 替代 pthread
 *   - constexpr cache_line_size
 *   - 结构体继承 aligned_base 保证对齐
 */

#include <cstdio>
#include <cstdint>
#include <atomic>
#include <thread>
#include <vector>
#include <chrono>

static constexpr int NTHREADS = 4;
static constexpr int NLOOP    = 100000000;
static constexpr size_t CACHE_LINE = 64;  // 绝大多数 x86/ARM

// ---------- 版本 1: 伪共享 ----------
struct CountersBad {
    std::atomic<uint64_t> c1{0};
    std::atomic<uint64_t> c2{0};
};  // sizeof = 16B, 同一 cache line

static CountersBad bad;

// ---------- 版本 2: alignas(64) 消除伪共享 ----------
struct alignas(64) AlignedCounter {
    std::atomic<uint64_t> value{0};
    // alignas(64) 自动填充到 64B 边界
};

struct CountersGood {
    AlignedCounter c1;
    AlignedCounter c2;
};  // sizeof = 128B, 各独占 cache line

static CountersGood good;

// ---------- 版本 3: per-thread 私有 ----------
struct alignas(64) PerThreadCounter {
    std::atomic<uint64_t> value{0};
};

static PerThreadCounter per_thread[NTHREADS];

// ---------- 计时 ----------
static double now_sec()
{
    using namespace std::chrono;
    return duration<double>(steady_clock::now().time_since_epoch()).count();
}

template<typename Fn1, typename Fn2>
static void run2(Fn1 fn1, Fn2 fn2, const char* label)
{
    double t0 = now_sec();
    std::thread t1(fn1);
    std::thread t2(fn2);
    t1.join();
    t2.join();
    double elapsed = now_sec() - t0;
    printf("  %-44s  %8.3f ms\n", label, elapsed * 1000.0);
}

int main()
{
    printf("=== Hennessy Ch2/5 · 伪共享对比 C++ (CACHE_LINE=%zu) ===\n\n",
           CACHE_LINE);

    printf("--- 2 线程, 各写不同计数器, %d M 次 ---\n\n", NLOOP / 1000000);

    printf("sizeof(CountersBad)  = %zu  (c1,c2 同 cache line)\n", sizeof(CountersBad));
    printf("sizeof(CountersGood) = %zu  (c1,c2 各独占 cache line)\n\n",
           sizeof(CountersGood));
    printf("sizeof(AlignedCounter) = %zu  (alignas(64))\n\n", sizeof(AlignedCounter));

    // 伪共享
    run2(
        []() { for (int i = 0; i < NLOOP; i++) bad.c1.fetch_add(1, std::memory_order_relaxed); },
        []() { for (int i = 0; i < NLOOP; i++) bad.c2.fetch_add(1, std::memory_order_relaxed); },
        "伪共享 (c1,c2 同 line)"
    );

    // 消除伪共享
    run2(
        []() { for (int i = 0; i < NLOOP; i++) good.c1.value.fetch_add(1, std::memory_order_relaxed); },
        []() { for (int i = 0; i < NLOOP; i++) good.c2.value.fetch_add(1, std::memory_order_relaxed); },
        "alignas(64) padding (各独占)"
    );

    // per-thread 私有
    printf("\n--- %d 线程私有计数器, 最后合并 ---\n\n", NTHREADS);
    {
        std::vector<std::thread> threads;
        double t0 = now_sec();

        for (int i = 0; i < NTHREADS; i++) {
            threads.emplace_back([i]() {
                for (int j = 0; j < NLOOP; j++)
                    per_thread[i].value.fetch_add(1, std::memory_order_relaxed);
            });
        }
        for (auto& t : threads) t.join();

        uint64_t total = 0;
        for (int i = 0; i < NTHREADS; i++)
            total += per_thread[i].value.load();

        double elapsed = now_sec() - t0;
        printf("  %-44s  %8.3f ms  total=%lu\n",
               "per-thread 私有计数器", elapsed * 1000.0,
               (unsigned long)total);
    }

    printf("\nC++ 特有点:\n");
    printf("  - alignas(64): 标准属性, 比 __attribute__ 更可移植\n");
    printf("  - std::atomic<uint64_t>: 类型安全, 无需 _Atomic 修饰符\n");
    printf("  - lambda + std::thread: 无需独立函数, 内联定义线程逻辑\n");
    printf("  - memory_order_relaxed: 明确指定内存序 (只要求原子性, 不要求顺序)\n");
    printf("  - C++17 hardware_destructive_interference_size (理想值, 但实现不完整)\n");

    printf("\n  HFT: 订单簿统计、风控计数器必须避免伪共享\n");
    printf("       per-thread + 定期合并是 HFT 常用模式\n");

    return 0;
}
