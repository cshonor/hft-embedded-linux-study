#pragma once

#include <atomic>
#include <cstddef>
#include <new>
#include <type_traits>

/*
 * SPSC = Single Producer Single Consumer：一个线程只写，一个线程只读。
 *
 * 为什么不用互斥锁？锁会让线程去睡觉（上下文切换，微秒级），HFT 热路径受不了。
 * 这里用「固定数组 + 两个原子下标」：写之前看是不是满了，读之前看是不是空了。
 *
 * 环形：下标一直加，用 & (N-1) 折回数组开头。所以 N 必须是 2 的幂。
 *
 * head / tail 各占一条 cache line（alignas(64)）：
 * 若挤在同一 64 字节里，写 head 会把读者的 tail 从对方 CPU 缓存里挤出去，叫伪共享。
 *
 * 内存序（对应笔记 Ch7.2 / Ch7.5）：
 *   先把数据写进 slots_[...]，再 release 地 head++。
 *   读者 acquire 地读 head，才能保证看见刚才那份数据。
 *   反了顺序就会读到半成品。
 */
template <typename T, std::size_t N>
class SpscRing {
    static_assert((N & (N - 1)) == 0, "capacity must be power of 2");
    static_assert(std::is_trivially_copyable<T>::value, "T must be trivially copyable");

    static constexpr std::size_t kMask = N - 1;

    struct alignas(64) ProducerPad {
        std::atomic<std::size_t> head{0}; // 下一个要写的位置（累计值，不是 0..N-1）
    } prod_;

    struct alignas(64) ConsumerPad {
        std::atomic<std::size_t> tail{0}; // 下一个要读的位置
    } cons_;

    alignas(64) T slots_[N];

public:
    static constexpr std::size_t capacity() { return N; }

    // 满了返回 false，调用方自旋再试（demo 的背压）。
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
            return false; // 空
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
