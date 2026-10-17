# std::shared_mutex

## 读写锁

```cpp
#include <shared_mutex>

std::shared_mutex rw_mtx;

// 读操作：共享锁，多线程并发读
{
    std::shared_lock lk(rw_mtx);  // 共享读锁
    // 多个线程可以同时持有 shared_lock
    read_data();
}

// 写操作：独占锁
{
    std::unique_lock lk(rw_mtx);  // 独占写锁
    // 只有当前线程持有锁
    write_data();
}
```

## 与 shared_timed_mutex 的区别

```cpp
// C++14：shared_timed_mutex（支持超时）
std::shared_timed_mutex timed_mtx;
std::shared_lock<std::shared_timed_mutex> lk(timed_mtx,
    std::chrono::milliseconds(100));  // 超时获取

// C++17：shared_mutex（不支持超时，但更轻量）
std::shared_mutex mtx;
// 不支持 try_lock_for 等——但性能更好
```

C++17 的 `shared_mutex` 去掉了超时能力，实现可以更高效（不需要维护超时逻辑）。

## 性能考虑

```cpp
// shared_mutex 的开销：
// - shared_lock：原子计数器 increment（读者数）
// - unique_lock：等待所有读者释放，然后独占
// - shared_lock 释放：原子计数器 decrement

// 低竞争场景：shared_mutex 比 mutex 更慢
// （维护计数器和状态机的开销 > 并发读收益）

// 读远多于写：shared_mutex 收益大
// 读写频率相近：shared_mutex 可能更慢
```

## 写饥饿问题

```cpp
// 如果读者持续获取 shared_lock，写者可能一直等不到独占锁
// → 写饥饿

// 某些实现有写者优先策略，但标准不保证
// 如果写延迟敏感，考虑：
// 1. 用普通 mutex（写不会饥饿）
// 2. 用无锁数据结构
// 3. 限制读持有时间
```

## HFT 应用

```cpp
// 行情快照：多策略读、单线程写
class MarketSnapshot {
    mutable std::shared_mutex mtx;
    QuoteData data;
public:
    // 多策略并发读
    QuoteData get() const {
        std::shared_lock lk(mtx);
        return data;
    }
    // 行情线程写
    void update(QuoteData new_data) {
        std::unique_lock lk(mtx);
        data = std::move(new_data);
    }
};

// 注意：HFT 极高频场景 shared_mutex 的原子计数开销可能成为瓶颈
// 替代方案：无锁双缓冲、seqlock
```

## 自测题

1. `shared_mutex` 和 `shared_timed_mutex` 的区别？
2. 读写锁适合什么场景？不适合什么场景？
3. 写饥饿是什么？如何避免？
4. `shared_lock` 和 `unique_lock` 分别对应什么锁？
5. HFT 极高频场景为什么可能不用 `shared_mutex`？替代方案？

<details>
<summary>参考答案</summary>

1. `std::shared_mutex` 是 **C++17** 引入的读写锁，提供 `lock`/`unlock`（独占）与 `lock_shared`/`unlock_shared`（共享）。
`std::shared_timed_mutex` 是 **C++14** 就有的版本，在共享/独占之外还提供**带超时**的接口：`try_lock_for` / `try_lock_until` 以及 `try_lock_shared_for` / `try_lock_shared_until`。
需要超时就用 `shared_timed_mutex`；不需要超时时用 `shared_mutex` 更轻（少维护一套超时机制）。
2. **适合**：读远多于写、且读临界区有一定长度的场景（如行情快照：多策略并发读、单线程更新）。多个读者可以真正并行，吞吐提升明显。
**不适合**：读写频率接近、或临界区极短的场景——共享锁要维护读者计数/状态机，原子操作与 cache 行竞争的开销可能超过并发读的收益，反而比普通 `std::mutex` 更慢。也不适合写延迟敏感的场景（见写饥饿）。
3. 写饥饿：读者源源不断地获取共享锁，导致写者始终拿不到独占锁，写操作被无限期推迟。
标准**不保证**任何写者优先策略（具体实现可能做写者优先，也可能不做）。规避办法：缩短读临界区持有时间、限制读者并发度、改用普通 `mutex`（写不会饥饿但读不能并行）、改用无锁结构、或用双缓冲/seqlock 让写不必等读者。
4. `std::shared_lock<std::shared_mutex>` 对应**共享锁（读锁）**：构造时 `lock_shared()`，析构时 `unlock_shared()`，可被多个线程同时持有。
`std::unique_lock<std::shared_mutex>` 对应**独占锁（写锁）**：构造时 `lock()`，析构时 `unlock()`，同一时刻只能有一个持有者。
两者都是 RAII，也都支持 `defer_lock` / `try_lock` / `std::adopt_lock`；`unique_lock` 的可移动性与手动 `unlock()` 让它还能配合条件变量使用。
5. 因为共享锁的获取/释放都要对共享计数器做原子 RMW，这个计数器是全局竞争点：读者越多，同一条 cache line 上的乒乓越严重，延迟抖动也随之增大；再加上写饥饿风险，尾延迟不可控。
替代方案：**无锁双缓冲**（写者写后台缓冲，原子切换指针，读者读不变快照）、**seqlock / 版本号读**（读者无锁读、发现版本变化重试）、**RCU**、或按核分片（每个核一份快照，读者无竞争）。这些都是「读者不写共享状态」的思路，能消除计数器竞争。

</details>
