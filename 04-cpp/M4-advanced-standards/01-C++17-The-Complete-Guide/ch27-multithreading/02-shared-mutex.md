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
