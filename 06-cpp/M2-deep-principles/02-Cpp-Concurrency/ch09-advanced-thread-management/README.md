# 第 9 章 高级线程管理

**Advanced Thread Management**

## 本章讲什么

手写 `std::thread` 适合简单场景，复杂系统需要线程池、任务队列、线程中断机制。本章讲线程池的设计演进、work-stealing 调度、`thread_local` 的高级用法、以及 C++20 协程（`std::jthread` + `stop_token`）的协作式中断。

## 要点

### 线程池基础

```cpp
class thread_pool {
    std::vector<std::thread> workers;
    std::queue<std::function<void()>> tasks;
    std::mutex m;
    std::condition_variable cv;
    bool stop = false;
public:
    thread_pool(size_t n) {
        for (size_t i = 0; i < n; ++i)
            workers.emplace_back([this]{
                for (;;) {
                    std::function<void()> task;
                    {
                        std::unique_lock<std::mutex> lk(m);
                        cv.wait(lk, [this]{ return stop || !tasks.empty(); });
                        if (stop && tasks.empty()) return;
                        task = std::move(tasks.front());
                        tasks.pop();
                    }
                    task();
                }
            });
    }
    ~thread_pool() {
        { std::lock_guard<std::mutex> lk(m); stop = true; }
        cv.notify_all();
        for (auto& t : workers) t.join();
    }
    template <class F> void submit(F&& f) {
        { std::lock_guard<std::mutex> lk(m); tasks.emplace(std::forward<F>(f)); }
        cv.notify_one();
    }
};
```

**设计要点**：
- 任务队列用 mutex+cv 保护，`function<void()>` 类型擦除任意可调用对象。
- 析构时设 stop=true 再 notify，让在排队的任务跑完才退出（graceful shutdown）。
- 返回值：用 `packaged_task` 包装，返回 `future` 让调用方取结果。

### Work-Stealing 调度

简单线程池有全局任务队列，成为锁竞争瓶颈。**Work-stealing**：每个 worker 有自己的本地队列（无锁或低竞争），空闲时去偷别人的任务。

```
Worker 0 本地队列: [A, B, C]    Worker 1 本地队列: []
Worker 1 偷 Worker 0 的 C → 执行 C
```

优点：本地队列几乎无竞争（只有自己 push/pop，别人 steal 是稀有事件），扩展性好。Intel TBB、Java ForkJoinPool 都用这个思路。

### `std::jthread`（C++20）与协作式中断

```cpp
std::jthread t([](std::stop_token st){
    while (!st.stop_requested()) {
        do_work();
    }
});
t.request_stop();   // 协作式：线程自己检查 stop_token 决定何时退
```

| 特性 | `std::thread` | `std::jthread` |
|------|---------------|----------------|
| 析构行为 | 未 join/detach → `terminate` | 自动 join（RAII） |
| 中断 | 无（只能设共享 flag） | `request_stop()` + `stop_token` |
| 安全性 | 易错（忘 join 就崩） | 默认安全 |

`stop_token` 是**协作式中断**——请求方设标志，线程自己检查决定退出点。不能强制杀死线程（强制杀会导致资源泄漏、锁未释放）。

### `stop_callback`（C++20）

```cpp
std::stop_source ss;
std::stop_callback cb(ss.get_token(), []{
    std::cout << "stopped!\n";
});
ss.request_stop();   // 触发回调
```

注册一个在 `request_stop()` 时自动执行的回调，用于清理资源。

### 协程（C++20，简介）

```cpp
std::future<int> async_work() {
    co_await something_async();
    co_return 42;
}
```

协程可以**挂起和恢复**，让异步代码写起来像同步。但 C++20 只给了底层机制（`co_await`/`co_yield`/`co_return`），库支持要等 C++23/26 的 `std::execution`。实战中常用 `cppcoro` 或 `asio` 的协程。

## HFT 关联

- **线程池 vs 固定流水线**：HFT 热路径**不用通用线程池**——任务调度不确定性高、有锁竞争。用固定绑核的流水线线程，每线程职责固定，延迟可预测。
- **`jthread` 用于非热路径**：管理线程（监控、日志、策略热切换）用 `jthread` 安全，热路径用裸 `thread` 绑核。
- **stop_token 做策略热切换**：策略线程检查 `stop_token`，收到停止请求后优雅退出（排空队列、保存状态），再启动新策略。
- **work-stealing 慎用**：偷任务有跨核 cache 失效 + 原子操作开销，HFT 倾向静态绑定避免。
- **协程的潜力**：异步行情订阅用协程可以避免回调地狱（callback hell），但要注意协程挂起/恢复有状态机开销，纳秒级热路径仍用同步。
- **绑核隔离**：管理线程和热路径线程分到不同核，避免管理线程的 GC/IO 抖动影响热路径。

## 自测题

1. 简单线程池的全局任务队列有什么扩展性瓶颈？work-stealing 如何解决？
2. `std::jthread` 相比 `std::thread` 有什么优势？为什么析构不会 `terminate`？
3. `stop_token` 的协作式中断和强制杀线程有什么区别？为什么不能强制杀？
4. HFT 热路径为什么不用通用线程池？固定流水线有什么优势？
5. C++20 协程的核心关键字是什么？为什么 HFT 纳秒级热路径仍倾向同步？
