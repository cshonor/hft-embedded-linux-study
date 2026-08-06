# 附录 D C++ 线程库参考

**C++ Thread Library Reference**

## 本附录讲什么

C++ 并发标准库的速查参考。按 `<thread>`、`<mutex>`、`<atomic>`、`<future>`、`<condition_variable>`、`<semaphore>`、`<latch>`、`<barrier>` 分类，列出核心类型和接口。

## 要点

### `<thread>` 线程管理

| 类型/函数 | 说明 |
|-----------|------|
| `std::thread` | 线程类 |
| `std::jthread`（C++20） | 自动 join + `stop_token` |
| `std::this_thread::get_id()` | 当前线程 ID |
| `std::this_thread::sleep_for(d)` | 睡眠时长 |
| `std::this_thread::sleep_until(tp)` | 睡到时间点 |
| `std::this_thread::yield()` | 让出 CPU（提示调度器） |
| `std::thread::hardware_concurrency()` | 硬件并发数（核数） |

```cpp
std::thread t([]{ /* work */ });
t.join();    // 等待结束
t.detach();  // 分离（后台运行）
std::thread t2(std::move(t));  // 所有权转移
```

### `<mutex>` 互斥锁

| 类型 | 说明 |
|------|------|
| `std::mutex` | 基本互斥锁 |
| `std::recursive_mutex` | 可重入（同线程多次 lock） |
| `std::timed_mutex` | 支持 `try_lock_for` |
| `std::shared_mutex`（C++17） | 读写锁 |

| RAII 守卫 | 说明 |
|-----------|------|
| `std::lock_guard<M>` | 构造锁、析构解，最简 |
| `std::unique_lock<M>` | 可延迟（`defer_lock`）、可提前解、可转移 |
| `std::shared_lock<M>`（C++17） | 读锁（配合 `shared_mutex`） |
| `std::scoped_lock<...>`（C++17） | 同时锁多个，防死锁 |

| 函数 | 说明 |
|------|------|
| `std::lock(m1, m2, ...)` | 原子锁多个（避免死锁） |
| `std::try_lock(m1, m2, ...)` | 尝试锁多个，失败返回索引 |

### `<atomic>` 原子操作

| 模板/类型 | 说明 |
|-----------|------|
| `std::atomic<T>` | 通用原子 |
| `std::atomic_flag` | 最简原子布尔（保证无锁） |
| `std::atomic<bool>` | 原子布尔 |
| `std::atomic<intptr_t>` | 原子指针大小整数 |
| `std::atomic<T*>` | 原子指针 |
| `std::atomic_ref<T>`（C++20） | 对已有变量的原子引用 |

| 操作 | 说明 |
|------|------|
| `load(mo)` / `store(v, mo)` | 原子读/写 |
| `exchange(v, mo)` | 原子交换 |
| `compare_exchange_strong/exp_weak` | CAS |
| `fetch_add/sub/and/or/xor` | 原子算术/位运算（返回旧值） |
| `is_lock_free()` | 是否真正无锁 |
| `wait(v)` / `notify_one/all`（C++20） | 原子等待/通知 |

```cpp
std::atomic<int> x{0};
x.fetch_add(1, std::memory_order_relaxed);
int expected = 1;
x.compare_exchange_strong(expected, 2);  // if x==1 then x=2
```

### `<future>` 异步结果

| 类型 | 说明 |
|------|------|
| `std::future<T>` | 异步结果读取（一次性） |
| `std::shared_future<T>` | 共享结果（可多次 get） |
| `std::promise<T>` | 结果写入端 |
| `std::packaged_task<F>` | 包装可调用对象，自动绑 promise/future |
| `std::async(policy, fn, args...)` | 最简异步执行 |

| 启动策略 | 说明 |
|----------|------|
| `std::launch::async` | 立即新线程 |
| `std::launch::deferred` | 延迟到 get() |
| 默认（两者 OR） | 实现决定 |

### `<condition_variable>` 条件变量

| 类型 | 说明 |
|------|------|
| `std::condition_variable` | 配合 `unique_lock<mutex>` |
| `std::condition_variable_any` | 可配任意锁（更重） |

| 操作 | 说明 |
|------|------|
| `wait(lk, pred)` | 等待（带谓词防虚假唤醒） |
| `wait_for(lk, dur, pred)` | 超时等待 |
| `wait_until(lk, tp, pred)` | 等到时间点 |
| `notify_one()` | 唤醒一个 |
| `notify_all()` | 唤醒全部 |

### `<semaphore>` 信号量（C++20）

| 类型 | 说明 |
|------|------|
| `std::counting_semaphore<N>` | 计数信号量 |
| `std::binary_semaphore` | 二值（`counting_semaphore<1>`） |

| 操作 | 说明 |
|------|------|
| `acquire()` | 计数 -1（0 时阻塞） |
| `release(n)` | 计数 +n，唤醒等待者 |
| `try_acquire()` | 非阻塞尝试 |

### `<latch>` / `<barrier>` 屏障（C++20）

| 类型 | 说明 |
|------|------|
| `std::latch` | 一次性计数屏障，不可重置 |
| `std::barrier` | 可复用屏障 |

```cpp
std::latch done(N);
for (int i = 0; i < N; ++i)
    threads.emplace_back([&]{ work(); done.count_down(); });
done.wait();   // 等所有 N 个 count_down

std::barrier sync(N);
for (phase = 0; phase < P; ++phase) {
    work_phase(phase);
    sync.arrive_and_wait();  // 所有线程到齐才继续
}
```

### `<memory_order>` 内存序

```cpp
std::memory_order_relaxed     // 无同步
std::memory_order_consume     // 数据依赖（实践中≈acquire）
std::memory_order_acquire     // 读屏障
std::memory_order_release     // 写屏障
std::memory_order_acq_rel     // RMW 读写屏障
std::memory_order_seq_cst     // 全局总序（默认）
```

## HFT 速查

| 场景 | 推荐 API |
|------|----------|
| 热路径无锁队列 | `atomic<size_t>` + acquire/release |
| 统计计数器 | `atomic<T>::fetch_add(relaxed)` |
| 一次性初始化 | `std::call_once` 或 `static` 局部变量 |
| 阶段同步 | `std::latch`（一次性） |
| 分阶段并行 | `std::barrier`（可复用） |
| 非热路径互斥 | `std::scoped_lock` |
| 读写多场景 | `std::shared_mutex` + `shared_lock` |
| 异步任务 | `std::async(launch::async, ...)` |
| 协作停止 | `std::jthread` + `stop_token` |
| 自旋等待 | `std::this_thread::yield()` |

## 自测题

1. `lock_guard`、`unique_lock`、`scoped_lock` 各有什么特点和适用场景？
2. `compare_exchange_weak` 和 `strong` 的区别？`fetch_add` 返回什么？
3. C++20 的 `latch` 和 `barrier` 有什么区别？`counting_semaphore` 的 `acquire` 和 `release` 做什么？
4. `std::async` 的三种启动策略分别什么含义？默认策略有什么坑？
5. HFT 热路径无锁队列用哪些 `<atomic>` 接口？为什么用 acquire/release 而非 seq_cst？
