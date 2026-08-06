# 第 4 章 并发操作的同步

**Synchronizing Concurrent Operations**

## 本章讲什么

线程不只是"各跑各的"，往往需要等待某个条件、某个事件或某个结果。本章讲 `condition_variable` 的条件等待、`future`/`promise` 的一对一结果传递、`packaged_task` 与 `async` 的异步任务抽象、以及一次性事件 `latch`/`barrier`（C++20）。

## 要点

### 条件变量 `condition_variable`

```cpp
std::mutex m;
std::condition_variable cv;
bool ready = false;

// 等待方
std::unique_lock<std::mutex> lk(m);
cv.wait(lk, [&]{ return ready; });   // 虚假唤醒自动重检 lambda

// 通知方
{
    std::lock_guard<std::mutex> lk(m);
    ready = true;
}
cv.notify_one();   // 或 notify_all()
```

关键点：
- `wait` 必须带**谓词**形式 `cv.wait(lk, pred)`，否则虚假唤醒会让条件判断失效。
- 通知方改变共享状态必须在**锁内**完成，否则 `wait` 可能在检查与睡眠之间错过通知。
- `notify_one` 唤醒一个，`notify_all` 唤醒全部（多消费者用 one，广播用 all）。

### future / promise：一次性结果传递

```cpp
std::promise<int> p;
std::future<int> f = p.get_future();
std::thread([&]{ p.set_value(42); }).detach();
int v = f.get();   // 阻塞直到 set_value
```

| 组件 | 角色 |
|------|------|
| `promise<T>` | 写入端（设值或异常） |
| `future<T>` | 读取端（阻塞 `get`） |
| `shared_future<T>` | 多读者共享同一个结果 |
| `packaged_task` | 包装可调用对象，自动绑 promise/future |
| `async` | 最简封装：`async(policy, fn, args...)` 返回 future |

### `std::async` 启动策略

```cpp
auto f1 = std::async(std::launch::async, work);       // 立即新线程
auto f2 = std::async(std::launch::deferred, work);    // 延迟到 get() 才执行
auto f3 = std::async(work);                            // 默认：async | deferred
```

**默认策略的坑**：`async(fn)` 不保证立即执行，可能延后到 `f.get()` 才在同线程跑——这会破坏并发性，且 `get()` 阻塞时才发现。**显式写 `launch::async`** 才能确保新线程。

### future 的局限

- **只能取一次**：`get()` 后 future 失效，不能重复等。多读者要用 `shared_future`。
- **不能轮询**：标准库没有 `try_get`，要么阻塞 `get()`，要么 `wait_for`/`wait_until` 超时。
- **析构阻塞**：`async` 返回的 future 在析构时会阻塞等待线程结束——这是 `async` 的隐式同步行为。

### 超时等待

```cpp
if (cv.wait_for(lk, std::chrono::milliseconds(100), [&]{ return ready; }))
    // 条件满足
else
    // 超时
```

### C++20 新同步原语

| 原语 | 作用 |
|------|------|
| `std::latch` | 一次性计数屏障，`count_down` + `wait`，不可重置 |
| `std::barrier` | 可复用屏障，`arrive_and_wait`，适合分阶段并行 |
| `std::counting_semaphore` | 信号量，控制并发数 |
| `std::binary_semaphore` | 二值信号量（≈ mutex 但可由非所有者释放） |

## HFT 关联

- **条件变量 vs 自旋等待**：`condition_variable` 会 park 线程（让出 CPU），适合非热路径。HFT 热路径用 `atomic` + 自旋（`yield`/`pause`）避免上下文切换。
- **future 析构阻塞陷阱**：`async` 返回的 future 析构会 join，热路径上误用会导致隐式串行化。
- **latch 做阶段同步**：策略初始化多阶段用 `latch` 等所有 worker 就位再开闸，比手写 mutex+cv 简洁。
- **barrier 做批量处理**：分片行情处理用 `barrier` 同步各分片完成，再聚合。
- **避免 promise/future 在纳秒级热路径**：它们内部有共享状态 + atomic + 可能的堆分配，延迟不可控；热路径用 SPSC 队列 + 序列号更可控。

## 自测题

1. 为什么 `condition_variable::wait` 必须用谓词形式？虚假唤醒是什么？
2. `std::async` 默认启动策略的坑是什么？为什么热路径要显式写 `launch::async`？
3. `future::get()` 为什么只能调一次？多读者怎么办？
4. `async` 返回的 future 析构时会发生什么？这对热路径有什么影响？
5. C++20 的 `latch` 和 `barrier` 有什么区别？分别适合什么场景？
