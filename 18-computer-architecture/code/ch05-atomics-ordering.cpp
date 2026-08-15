/*
 * Hennessy Ch5 · 内存一致性模型 — C++ atomic + memory_order
 *
 * 对照笔记:
 *   03/chapter-05/notes/section-5.6-内存一致性模型.md
 *   03/chapter-05/notes/section-5.5-同步基础.md
 *
 * 编译:
 *   g++ -Wall -Wextra -std=c++17 -O2 -o ch05_ato_cpp ch05-atomics-ordering.cpp -lpthread
 * 运行:
 *   ./ch05_ato_cpp
 *
 * C++ 差异:
 *   - std::atomic<T> 替代 _Atomic T
 *   - std::memory_order_{relaxed,acquire,release,seq_cst}
 *   - std::atomic_flag (保证无锁的布尔, test-and-set)
 *   - 自旋锁用 atomic_flag::test_and_set + notify/wait (C++20)
 *   - 更直观的成员函数语法: a.store(v, order) / a.load(order)
 */

#include <cstdio>
#include <cstdint>
#include <atomic>
#include <thread>
#include <vector>

// ---------- 1. Message Passing ----------
struct Message {
    int payload[4];
    std::atomic<int> ready{0};
};

static Message msg;
static std::atomic<int> acquire_fail{0};
static std::atomic<int> relaxed_fail{0};

static constexpr int NTHREADS = 4;
static constexpr int ITERS    = 1000000;

void producer_release()
{
    msg.payload[0] = 42;
    msg.payload[1] = 99;
    msg.payload[2] = 7;
    msg.payload[3] = 13;
    // release: 之前的写对 acquire 可见
    msg.ready.store(1, std::memory_order_release);
}

void consumer_acquire()
{
    // acquire: 看到.ready==1 后, 之前的写都可见
    while (msg.ready.load(std::memory_order_acquire) == 0)
        ; // spin
    if (msg.payload[0] != 42 || msg.payload[1] != 99 ||
        msg.payload[2] != 7   || msg.payload[3] != 13)
        acquire_fail.fetch_add(1);
}

void producer_relaxed()
{
    msg.payload[0] = 42;
    msg.payload[1] = 99;
    msg.payload[2] = 7;
    msg.payload[3] = 13;
    // relaxed: 不保证顺序!
    msg.ready.store(1, std::memory_order_relaxed);
}

void consumer_relaxed()
{
    while (msg.ready.load(std::memory_order_relaxed) == 0)
        ;
    if (msg.payload[0] != 42 || msg.payload[1] != 99 ||
        msg.payload[2] != 7   || msg.payload[3] != 13)
        relaxed_fail.fetch_add(1);
}

// ---------- 2. 自旋锁 (atomic_flag) ----------
class SpinLock {
    std::atomic_flag flag = ATOMIC_FLAG_INIT;
public:
    void lock()
    {
        while (flag.test_and_set(std::memory_order_acquire))
            ; // spin — 生产环境加 pause/backoff
    }
    void unlock()
    {
        flag.clear(std::memory_order_release);
    }
};

static SpinLock slock;
static int shared_counter = 0;

void lock_worker()
{
    for (int i = 0; i < ITERS; i++) {
        std::lock_guard<SpinLock> lk(slock);  // RAII
        shared_counter++;
    }
}

// ---------- 3. 原子计数器 ----------
static std::atomic<uint64_t> counter{0};

void atomic_counter_worker()
{
    for (int i = 0; i < ITERS; i++)
        counter.fetch_add(1, std::memory_order_relaxed);
}

// ---------- 测试 ----------
static double now_sec()
{
    using namespace std::chrono;
    return duration<double>(steady_clock::now().time_since_epoch()).count();
}

int main()
{
    printf("=== Hennessy Ch5 · 内存一致性模型 C++ (std::atomic) ===\n\n");

    // Message Passing
    printf("--- 1. Message Passing (release-acquire vs relaxed) ---\n\n");

    constexpr int TRIALS = 10000;
    for (int i = 0; i < TRIALS; i++) {
        msg.ready.store(0);
        msg.payload[0] = msg.payload[1] = msg.payload[2] = msg.payload[3] = 0;

        std::thread p(producer_release);
        std::thread c(consumer_acquire);
        p.join(); c.join();
    }
    printf("  release-acquire: %d trials, %d failures (期望 0)\n",
           TRIALS, acquire_fail.load());

    for (int i = 0; i < TRIALS; i++) {
        msg.ready.store(0);
        msg.payload[0] = msg.payload[1] = msg.payload[2] = msg.payload[3] = 0;

        std::thread p(producer_relaxed);
        std::thread c(consumer_relaxed);
        p.join(); c.join();
    }
    printf("  relaxed:          %d trials, %d failures (x86 上通常 0)\n\n",
           TRIALS, relaxed_fail.load());

    // 自旋锁
    printf("--- 2. 自旋锁 (atomic_flag + RAII) ---\n\n");
    {
        std::vector<std::thread> threads;
        for (int i = 0; i < NTHREADS; i++)
            threads.emplace_back(lock_worker);
        for (auto& t : threads) t.join();
        printf("  自旋锁: counter=%d (期望=%d)\n\n",
               shared_counter, NTHREADS * ITERS);
    }

    // 原子计数器
    printf("--- 3. 原子计数器 (fetch_add relaxed) ---\n\n");
    {
        std::vector<std::thread> threads;
        for (int i = 0; i < NTHREADS; i++)
            threads.emplace_back(atomic_counter_worker);
        for (auto& t : threads) t.join();
        printf("  fetch_add: counter=%lu (期望=%d)\n\n",
               (unsigned long)counter.load(), NTHREADS * ITERS);
    }

    printf("std::memory_order 速查:\n");
    printf("  relaxed:     原子但不排序 — 计数器/统计可用\n");
    printf("  acquire:     load 后续操作不重排到前面 — 消费者读标志\n");
    printf("  release:     store 之前操作不重排到后面 — 生产者写标志\n");
    printf("  acq_rel:     同时 acquire+release — read-modify-write\n");
    printf("  seq_cst:     全局顺序一致 — 最强, 默认值, 开销最大\n");
    printf("\nC++ 特有点:\n");
    printf("  - std::atomic_flag: 保证无锁 (lock-free), 适合自旋锁\n");
    printf("  - lock_guard<SpinLock>: 用户定义锁类型 + RAII\n");
    printf("  - 成员函数语法: a.store(v, order) / a.load(order)\n");
    printf("  - C++20: atomic_ref, atomic<shared_ptr>, wait/notify\n");

    return 0;
}
