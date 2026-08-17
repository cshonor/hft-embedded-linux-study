#pragma once

#include <atomic>
#include <cstddef>
#include <new>
#include <type_traits>

// 单生产者单消费者无锁环。head/tail 分在两条 cache line 上，避免伪共享。
// 对应 14-hft-engineering Ch7.2：预分配固定数组 + acquire/release。
template <typename T, std::size_t N>
class SpscRing {
    static_assert((N & (N - 1)) == 0, "capacity must be power of 2");
    static_assert(std::is_trivially_copyable<T>::value, "T must be trivially copyable");

    static constexpr std::size_t kMask = N - 1;

    struct alignas(64) ProducerPad {
        std::atomic<std::size_t> head{0};
    } prod_;

    struct alignas(64) ConsumerPad {
        std::atomic<std::size_t> tail{0};
    } cons_;

    alignas(64) T slots_[N];

public:
    static constexpr std::size_t capacity() { return N; }

    bool try_push(const T& v) {
        const std::size_t h = prod_.head.load(std::memory_order_relaxed);
        const std::size_t t = cons_.tail.load(std::memory_order_acquire);
        if (h - t >= N) {
            return false;
        }
        slots_[h & kMask] = v;
        prod_.head.store(h + 1, std::memory_order_release);
        return true;
    }

    bool try_pop(T& out) {
        const std::size_t t = cons_.tail.load(std::memory_order_relaxed);
        const std::size_t h = prod_.head.load(std::memory_order_acquire);
        if (t == h) {
            return false;
        }
        out = slots_[t & kMask];
        cons_.tail.store(t + 1, std::memory_order_release);
        return true;
    }

    std::size_t size_approx() const {
        const std::size_t h = prod_.head.load(std::memory_order_acquire);
        const std::size_t t = cons_.tail.load(std::memory_order_acquire);
        return h - t;
    }
};
