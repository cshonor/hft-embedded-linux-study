# D.2 mutex 互斥锁

> 附录 D · 上一节：[D.1 thread 线程管理](01-thread.md) · 下一节：[D.3 atomic 原子操作](03-atomic.md)

## 这节讲什么

`<mutex>` / `<shared_mutex>` 头文件提供互斥锁设施。本节是速查参考——各种 mutex 类型、RAII 守卫、以及 `scoped_lock` 的死锁 avoidance。

---

## 核心规则（代码+表格）

### Mutex 类型

| 类型 | 头文件 | 特性 |
|------|--------|------|
| `std::mutex` | `<mutex>` | 基本互斥锁 |
| `std::recursive_mutex` | `<mutex>` | 可重入（同线程多次 lock） |
| `std::timed_mutex` | `<mutex>` | 支持 `try_lock_for` |
| `std::recursive_timed_mutex` | `<mutex>` | 可重入 + 超时 |
| `std::shared_mutex` | `<shared_mutex>` | 读写锁（C++17） |

```cpp
std::mutex m;
m.lock();
// 临界区
m.unlock();

// 同线程多次 lock：recursive_mutex 可以，mutex 死锁
std::recursive_mutex rm;
rm.lock();
rm.lock();  // OK，count=2
rm.unlock();
rm.unlock();  // count=0

// 超时
std::timed_mutex tm;
if (tm.try_lock_for(100ms)) {
    // 拿到锁
    tm.unlock();
} else {
    // 超时
}

// 读写锁
std::shared_mutex sm;
sm.lock();         // 写锁（独占）
sm.unlock();
sm.lock_shared();  // 读锁（共享）
sm.unlock_shared();
```

### RAII 守卫

| 守卫 | 头文件 | 用途 |
|------|--------|------|
| `lock_guard<M>` | `<mutex>` | 简单作用域锁 |
| `unique_lock<M>` | `<mutex>` | 灵活（可解锁/移动/条件变量） |
| `shared_lock<M>` | `<shared_mutex>` | 读锁（C++14） |
| `scoped_lock<...>` | `<mutex>` | 多锁原子获取（C++17） |

```cpp
std::mutex m;

// lock_guard：最简单
{
    std::lock_guard<std::mutex> lk(m);
    // 临界区
}  // 析构自动 unlock

// unique_lock：灵活
{
    std::unique_lock<std::mutex> lk(m);
    // 临界区
    lk.unlock();  // 手动解锁
    // 非临界区
    lk.lock();    // 重新加锁
    // 临界区
}  // 析构自动 unlock（如果还锁着）

// shared_lock：读锁
std::shared_mutex sm;
{
    std::shared_lock<std::shared_mutex> rlk(sm);  // 读锁
    // 多线程可同时进入
}
{
    std::unique_lock<std::shared_mutex> wlk(sm);  // 写锁（独占）
    // 只有本线程
}

// scoped_lock：多锁原子获取（无死锁）
std::mutex m1, m2;
{
    std::scoped_lock lk(m1, m2);  // 原子获取两把锁
    // 临界区
}
```

### `unique_lock` 与条件变量

```cpp
std::mutex m;
std::condition_variable cv;
bool ready = false;

// 消费者
std::unique_lock<std::mutex> lk(m);
cv.wait(lk, [&]{ return ready; });  // unique_lock 可配合 cv
// lk 仍持有锁

// 生产者
{
    std::lock_guard<std::mutex> lk(m);
    ready = true;
}
cv.notify_one();
```

### `call_once` 一次性初始化

```cpp
std::once_flag flag;
void init() { /* 只执行一次 */ }

void worker() {
    std::call_once(flag, init);  // 多线程调用，init 只执行一次
}
```

### `std::try_lock` 多锁尝试

```cpp
std::mutex m1, m2, m3;
// 尝试同时锁三把锁（非原子，可能部分成功）
int result = std::try_lock(m1, m2, m3);
if (result == -1) {
    // 全部成功
    // ...
    m1.unlock(); m2.unlock(); m3.unlock();
} else {
    // result 是失败的第几把锁（0-indexed）
    // 已成功的锁自动解锁
}
```

---

## 新手要点（和 C 的区别）

- **C 用 `pthread_mutex_t` + 手动 lock/unlock**：C++ 的 RAII 守卫让锁管理更安全——析构自动 unlock，不会忘记。C 程序员转型时要改掉手动 lock/unlock 的习惯。
- **`recursive_mutex` 是 C 也有但少用的**：C 的 `pthread_mutex_t` 可以设 `PTHREAD_MUTEX_RECURSIVE`。C++ 的 `recursive_mutex` 更直观。但递归锁通常表示设计问题——尽量不用。
- **`scoped_lock` 是 C++17 的新利器**：C 程序员多锁要小心锁序——`scoped_lock` 用死锁 avoidance 算法原子获取，无需手动锁序。这是 C++ 相比 C 的巨大优势。
- **`shared_mutex` 等价于 `pthread_rwlock_t`**：C 程序员如果用过读写锁，理解 `shared_mutex` 容易。C++17 的 `shared_lock` 是 RAII 读锁——比 C 的手动 `rdlock`/`unlock` 安全。

---

## HFT 关联

- **HFT 热路径避免 mutex**：mutex 竞争时走 futex（系统调用），延迟微秒级——HFT 热路径用 atomic 或无锁队列。
- **`scoped_lock` 用于管理面**：HFT 管理面如果必须持有多把锁，用 `scoped_lock`——避免死锁。
- **`shared_mutex` 用于行情快照表**：多策略线程读、一个线程写——`shared_mutex` 让多读并行。
- **`call_once` 用于初始化**：HFT 系统的全局初始化（如加载配置）用 `call_once`——比 Meyers Singleton 更显式。
- **`timed_mutex` 用于超时检测**：HFT 的某些场景（如等锁超时告警）用 `timed_mutex`——但不能用于热路径。

---

## 自测题

1. `lock_guard` 和 `unique_lock` 有什么区别？各自用在哪里？
2. `scoped_lock` 如何避免多锁死锁？
3. `shared_mutex` 的读锁和写锁分别用什么 RAII 守卫？
4. `recursive_mutex` 什么时候用？为什么不推荐？
5. `call_once` 解决了什么问题？

<details>
<summary>参考答案</summary>

1. `lock_guard` 轻量地在构造时锁定、析构时解锁，适合整个作用域持锁且无需手动控制。
   `unique_lock` 可延迟锁定、解锁/重锁、移动并配合 condition_variable，功能更多且对象状态更复杂。
2. `scoped_lock` 接收多个 mutex，并采用与 `std::lock` 等价的死锁避免方式取得全部锁。
   成功后由 RAII 一并释放；标准不规定固定内部算法，调用方也不能在别处违反整体锁协议。
3. 读锁使用 `std::shared_lock<std::shared_mutex>`，允许多个读者并发持有。
   写锁使用 `std::unique_lock<std::shared_mutex>` 或 `lock_guard`，独占访问受保护状态。
4. 仅在同一线程确实需要重入同一把锁、且短期无法重构递归调用关系时使用 `recursive_mutex`。
   它会掩盖锁层次和职责问题、增加开销，并不能解决跨线程死锁，通常应拆分已持锁/未持锁函数。
5. `std::call_once` 配合 `once_flag` 保证某个初始化函数在并发调用中成功执行一次，并安全发布结果。
   若该函数抛异常，此次不算完成，后续调用会重试，避免手写一次性初始化的竞争。

</details>

---

## 参考与延伸

- 下一节：[D.3 atomic 原子操作](03-atomic.md)
- 上一节：[D.1 thread 线程管理](01-thread.md)
- 回到：[附录 D](README.md)
