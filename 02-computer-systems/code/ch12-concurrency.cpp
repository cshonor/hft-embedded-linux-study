/*
 * CSAPP Ch12 · 并发编程 — C++ 版 (std::thread + std::mutex + std::atomic)
 *
 * 对照笔记:
 *   chapter-12/notes/section-12.3-基于线程的并发编程.md
 *   chapter-12/notes/section-12.5-信号量与预线程化.md
 *
 * 编译:
 *   g++ -Wall -Wextra -std=c++17 -O2 -o ch12_conc_cpp ch12-concurrency.cpp -lpthread
 * 运行:
 *   ./ch12_conc_cpp
 *
 * C++ 差异:
 *   - std::thread 替代 pthread_create (跨平台)
 *   - std::mutex / std::lock_guard (RAII 自动解锁)
 *   - std::atomic 替代 C11 stdatomic.h
 *   - std::condition_variable 替代 sem_t (C++ 无标准信号量, C++20 才有)
 *   - lambda 创建线程函数更方便
 *   - std::scoped_lock / std::unique_lock 多锁安全
 */

#include <cstdio>
#include <thread>
#include <mutex>
#include <atomic>
#include <condition_variable>
#include <vector>
#include <queue>

static constexpr int NTHREADS = 4;
static constexpr int NLOOP    = 1000000;

// ---------- 1. 竞态条件演示 ----------
static int counter_unsafe = 0;

static void race_unsafe()
{
    for (int i = 0; i < NLOOP; i++)
        counter_unsafe++;  // 非原子
}

// ---------- 2. std::mutex + lock_guard ----------
static int counter_mutex = 0;
static std::mutex mtx;

static void race_mutex()
{
    for (int i = 0; i < NLOOP; i++) {
        std::lock_guard<std::mutex> lk(mtx);  // RAII: 离开作用域自动解锁
        counter_mutex++;
    }
}

// ---------- 3. std::atomic ----------
static std::atomic<int> counter_atomic{0};

static void race_atomic()
{
    for (int i = 0; i < NLOOP; i++)
        counter_atomic.fetch_add(1, std::memory_order_relaxed);
}

// ---------- 4. 生产者-消费者 (condition_variable) ----------
template<typename T>
class BlockingQueue {
    std::queue<T>           queue_;
    std::mutex              mtx_;
    std::condition_variable cv_not_empty_;
    std::condition_variable cv_not_full_;
    size_t                  capacity_;
    bool                    done_ = false;
public:
    explicit BlockingQueue(size_t cap) : capacity_(cap) {}

    void push(T item)
    {
        std::unique_lock<std::mutex> lk(mtx_);
        cv_not_full_.wait(lk, [this] { return queue_.size() < capacity_ || done_; });
        if (done_) return;
        queue_.push(std::move(item));
        cv_not_empty_.notify_one();
    }

    bool pop(T& out)
    {
        std::unique_lock<std::mutex> lk(mtx_);
        cv_not_empty_.wait(lk, [this] { return !queue_.empty() || done_; });
        if (queue_.empty()) return false;
        out = std::move(queue_.front());
        queue_.pop();
        cv_not_full_.notify_one();
        return true;
    }

    void shutdown()
    {
        std::lock_guard<std::mutex> lk(mtx_);
        done_ = true;
        cv_not_empty_.notify_all();
        cv_not_full_.notify_all();
    }
};

// ---------- 运行测试 ----------
template<typename Fn>
static void run_threads(Fn fn, int nthreads, const char* label, int expected)
{
    std::vector<std::thread> threads;
    for (int i = 0; i < nthreads; i++)
        threads.emplace_back(fn);
    for (auto& t : threads)
        t.join();
    // 由调用方打印 result
}

int main()
{
    printf("=== CSAPP Ch12 · 并发编程 C++ (std::thread) ===\n\n");

    // 竞态对比
    printf("--- 1. 竞态条件对比 (%d threads × %d loops) ---\n\n", NTHREADS, NLOOP);

    {
        std::vector<std::thread> ts;
        for (int i = 0; i < NTHREADS; i++) ts.emplace_back(race_unsafe);
        for (auto& t : ts) t.join();
        printf("  %-28s  result=%d  (期望=%d)\n",
               "无保护 (有竞态)", counter_unsafe, NTHREADS * NLOOP);
    }
    {
        std::vector<std::thread> ts;
        for (int i = 0; i < NTHREADS; i++) ts.emplace_back(race_mutex);
        for (auto& t : ts) t.join();
        printf("  %-28s  result=%d  (期望=%d)\n",
               "std::mutex + lock_guard", counter_mutex, NTHREADS * NLOOP);
    }
    {
        std::vector<std::thread> ts;
        for (int i = 0; i < NTHREADS; i++) ts.emplace_back(race_atomic);
        for (auto& t : ts) t.join();
        printf("  %-28s  result=%d  (期望=%d)\n",
               "std::atomic (relaxed)", counter_atomic.load(),
               NTHREADS * NLOOP);
    }

    printf("\n  lock_guard: RAII — 构造加锁, 析构解锁, 异常安全\n");
    printf("  atomic: 无锁, memory_order_relaxed 最快但不保证顺序\n\n");

    // 生产者-消费者
    printf("--- 2. 生产者-消费者 (BlockingQueue<T>) ---\n\n");

    BlockingQueue<int> bq(8);
    constexpr int NITEMS = 1000;

    auto producer = [&](int id) {
        for (int i = 0; i < NITEMS; i++) {
            int item = id * 1000 + i;
            bq.push(item);
            if (i % 200 == 0)
                printf("  [P%d] 生产 item=%d\n", id, item);
        }
    };

    auto consumer = [&](int id) {
        for (int i = 0; i < NITEMS; i++) {
            int item;
            if (bq.pop(item) && i % 200 == 0)
                printf("  [C%d] 消费 item=%d\n", id, item);
        }
    };

    std::vector<std::thread> prods, cons;
    for (int i = 0; i < 2; i++) prods.emplace_back(producer, i + 1);
    for (int i = 0; i < 2; i++) cons.emplace_back(consumer, i + 1);

    for (auto& t : prods) t.join();
    bq.shutdown();
    for (auto& t : cons) t.join();

    printf("\nC++ 特有点:\n");
    printf("  - std::thread: 跨平台, 无需 -lpthread 以外的平台 API\n");
    printf("  - lock_guard/unique_lock: RAII 自动解锁, 异常安全\n");
    printf("  - std::atomic: C++11 原生, 支持 memory_order\n");
    printf("  - BlockingQueue<T>: 模板化, 泛型安全\n");
    printf("  - condition_variable: 替代 POSIX sem_t (C++20 才有 std::counting_semaphore)\n");
    printf("  - lambda + emplace_back: 线程创建简洁\n");

    return 0;
}
