/*
 * Hennessy Ch5 · 无锁 SPSC 环形队列 — C++ std::atomic
 *
 * 对照笔记:
 *   03/chapter-05/notes/section-5.6-内存一致性模型.md
 *   03/chapter-05/notes/section-5.5-同步基础.md
 *
 * 编译:
 *   g++ -Wall -Wextra -std=c++17 -O2 -o ch05_spsc_cpp ch05-spsc-ringbuf.cpp -lpthread
 * 运行:
 *   ./ch05_spsc_cpp
 *
 * C++ 差异:
 *   - std::atomic<uint64_t> 替代 _Atomic uint64_t
 *   - alignas(64) 隔离 head/tail 到不同 cache line (避免伪共享)
 *   - 模板化 SpscQueue<T> — 类型安全
 *   - RAII: 析构无需手动释放
 *   - constexpr capacity
 */

#include <cstdio>
#include <cstdint>
#include <atomic>
#include <thread>
#include <chrono>
#include <cstring>

// ---------- 模板化 SPSC 环形队列 ----------
template<typename T, size_t Capacity>
class SpscQueue {
    static_assert((Capacity & (Capacity - 1)) == 0,
                  "Capacity must be power of 2");

    static constexpr size_t MASK = Capacity - 1;

    // cache-line 对齐, 避免 head/tail 伪共享
    alignas(64) std::atomic<uint64_t> head_{0};
    alignas(64) std::atomic<uint64_t> tail_{0};
    alignas(64) T slots_[Capacity];

public:
    SpscQueue() { head_.store(0); tail_.store(0); }

    // 生产者调用 (单线程)
    bool push(const T& item)
    {
        uint64_t head = head_.load(std::memory_order_relaxed);
        uint64_t tail = tail_.load(std::memory_order_acquire);
        // acquire: 消费者已读完的槽位可见

        if (head - tail >= Capacity)
            return false;  // 满

        slots_[head & MASK] = item;

        // release: slots 写入对消费者可见
        head_.store(head + 1, std::memory_order_release);
        return true;
    }

    // 消费者调用 (单线程)
    bool pop(T& out)
    {
        uint64_t tail = tail_.load(std::memory_order_relaxed);
        uint64_t head = head_.load(std::memory_order_acquire);
        // acquire: 生产者写入的 slots 可见

        if (head == tail)
            return false;  // 空

        out = slots_[tail & MASK];

        // release: 读取完成, 生产者可复用
        tail_.store(tail + 1, std::memory_order_release);
        return true;
    }

    // 仅生产者调用
    uint64_t size_approx() const
    {
        return head_.load(std::memory_order_relaxed) -
               tail_.load(std::memory_order_relaxed);
    }
};

// ---------- 测试 ----------
static constexpr size_t CAP = 1024;
static constexpr uint64_t NITEMS = 10000000;
static std::atomic<bool> producer_done{false};

int main()
{
    printf("=== Hennessy Ch5 · 无锁 SPSC 环形队列 C++ (std::atomic) ===\n\n");
    printf("  Capacity=%zu (power of 2, & MASK)\n\n", CAP);

    // 1. FIFO 顺序验证
    printf("--- 1. FIFO 顺序验证 ---\n\n");

    SpscQueue<int, 16> q;
    for (int i = 0; i < 10; i++)
        q.push(i);

    int val;
    int expected = 0;
    bool ok = true;
    while (q.pop(val)) {
        if (val != expected) {
            printf("  顺序错误: 期望 %d, 实际 %d\n", expected, val);
            ok = false;
        }
        expected++;
    }
    printf("  入队 10 项, 出队 %d 项, 顺序%s\n\n", expected, ok ? "正确 ✓" : "错误 ✗");

    // 2. 吞吐量测试
    printf("--- 2. 吞吐量测试 (%lu 万 items) ---\n\n", NITEMS / 10000);

    SpscQueue<uint64_t, CAP> queue;
    std::atomic<uint64_t> enqueued{0};
    std::atomic<uint64_t> dequeued{0};

    auto producer = [&]() {
        uint64_t count = 0;
        for (uint64_t i = 0; i < NITEMS; i++) {
            while (!queue.push(i))
                ; // spin
            count++;
        }
        producer_done.store(true);
        enqueued.store(count);
    };

    auto consumer = [&]() {
        uint64_t count = 0;
        uint64_t val;
        while (true) {
            if (queue.pop(val)) {
                count++;
            } else {
                if (producer_done.load() && !queue.pop(val))
                    break;
            }
        }
        dequeued.store(count);
    };

    auto t0 = std::chrono::steady_clock::now();

    std::thread prod(producer);
    std::thread cons(consumer);
    prod.join();
    cons.join();

    auto t1 = std::chrono::steady_clock::now();
    double elapsed = std::chrono::duration<double>(t1 - t0).count();

    printf("  入队: %lu\n", (unsigned long)enqueued.load());
    printf("  出队: %lu\n", (unsigned long)dequeued.load());
    printf("  耗时: %.3f ms\n", elapsed * 1000.0);
    printf("  吞吐: %.1f M ops/sec\n",
           (double)NITEMS / elapsed / 1e6);

    printf("\n内存序关键:\n");
    printf("  push:  写 slots[head] → release store head+1\n");
    printf("  pop:   acquire load head → 读 slots[tail] → release store tail+1\n");
    printf("  alignas(64): head/tail/slots 各占独立 cache line, 无伪共享\n");

    printf("\nC++ 特有点:\n");
    printf("  - 模板 SpscQueue<T, Capacity>: 类型安全, 编译期检查\n");
    printf("  - static_assert: 编译期保证 Capacity 是 2 的幂\n");
    printf("  - alignas(64): 标准属性, 跨平台 cache-line 对齐\n");
    printf("  - RAII: 无需 init/destroy, 构造即就绪\n");
    printf("  - 无 reinterpret_cast: T 直接存储, 无 void* 转换\n");
    printf("\n  HFT pipeline: NIC RX → SPSC → 策略 → SPSC → 下单\n");

    return 0;
}
