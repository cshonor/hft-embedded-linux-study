# 9.4 stop_callback

> 第 9 章 · 上一节：[9.3 jthread 与协作式中断](03-jthread.md) · 下一节：[9.5 协程简介](05-coroutine.md)

## 这节讲什么

`std::stop_callback`（C++20）允许在线程的 `stop_token` 上注册回调——当 `request_stop()` 被调用时，回调同步执行。本节讲它的使用模式、回调执行时机、以及典型应用场景（通知、清理、取消等待）。

---

## 核心规则（代码+表格）

### 基本用法

```cpp
#include <stop_token>
#include <iostream>

void worker(std::stop_token st) {
    // 注册回调：request_stop() 时自动调用
    std::stop_callback cb1(st, []{
        std::cout << "callback 1: cleaning up\n";
    });

    // 可以注册多个回调
    std::stop_callback cb2(st, []{
        std::cout << "callback 2: notifying manager\n";
    });

    // 回调的析构会注销（如果还没执行的话）
    {
        std::stop_callback temp(st, []{
            std::cout << "temp callback\n";
        });
        // temp 析构 → 注销，永远不会执行
    }

    while (!st.stop_requested()) {
        work();
    }
    std::cout << "worker exiting\n";
}

// 外部 request_stop() 时：
// 1. cb1 和 cb2 同步执行（在 request_stop() 的调用线程中）
// 2. 然后工作线程的 st.stop_requested() 变 true
```

### 回调执行时机

```cpp
std::jthread t(worker);

// 主线程调用 request_stop()
t.request_stop();
// 此时：在主线程中同步执行 cb1、cb2
// cb1、cb2 执行完毕后，request_stop() 才返回
// 然后工作线程看到 stop_requested()=true，退出循环
```

| 时机 | 说明 |
|------|------|
| 注册时 stop 已请求 | 回调**立即**在注册线程中执行 |
| 注册后 request_stop() | 回调在 **request_stop() 调用线程**中同步执行 |
| 回调对象析构（未执行） | 注销，不再执行 |
| 回调对象析构（已执行） | 无操作（已执行完） |

### 应用场景：取消阻塞等待

```cpp
// 问题：worker 在 condition_variable 上阻塞，request_stop 无法唤醒它
void worker_blocking(std::stop_token st) {
    std::mutex m;
    std::condition_variable cv;
    bool ready = false;

    // 用 stop_callback 唤醒阻塞的 wait
    std::stop_callback cb(st, [&]{
        {
            std::lock_guard<std::mutex> lk(m);
            ready = true;  // 让 wait 的谓词为 true
        }
        cv.notify_one();  // 唤醒 worker
    });

    std::unique_lock<std::mutex> lk(m);
    cv.wait(lk, [&]{ return st.stop_requested() || ready; });
    // request_stop() → cb 执行 → notify → wait 返回
}
```

### 应用场景：通知其他组件

```cpp
class StrategyEngine {
    std::jthread worker_thread;
    std::atomic<bool> strategy_running{false};

public:
    void start() {
        worker_thread = std::jthread([this](std::stop_token st){
            std::stop_callback cb(st, [this]{
                strategy_running = false;  // 通知策略已停止
                notify_risk_system();      // 通知风控系统
            });
            strategy_running = true;
            while (!st.stop_requested()) {
                process_tick();
            }
        });
    }
    void stop() { worker_thread.request_stop(); }
};
```

---

## 新手要点（和 C 的区别）

- **C 没有等价机制**：C 的 `pthread_cleanup_push`/`pthread_cleanup_pop` 类似但语义不同（在线程退出时执行，而非 stop 请求时）。`stop_callback` 是"请求停止时执行"，更精确。
- **回调在 request_stop() 线程中执行**：C 程序员可能以为回调在工作线程中执行——不是。它在调用 `request_stop()` 的线程中同步执行。这意味着回调不能太重（会阻塞 request_stop()）。
- **RAII 注销是关键**：`stop_callback` 的析构会注销回调——如果回调还没执行。这让回调的生命周期可控。C 程序员用 `pthread_cleanup_push` 要配对 `pop`，容易出错。C++ 的 RAII 自动处理。
- **注册时 stop 已请求则立即执行**：这个语义很重要——如果先 `request_stop()` 再注册回调，回调立即执行。C 程序员可能没想到这个"立即执行"的行为。

---

## HFT 关联

- **HFT 系统关机时的通知链**：`stop_callback` 让关机流程简洁——策略线程注册"通知风控"和"通知下单模块"的回调，`request_stop()` 时自动按顺序执行，无需手动编排。
- **唤醒阻塞操作**：HFT 线程可能在 `condition_variable` 上等待下一个 tick。`stop_callback` 可以在 `request_stop()` 时 notify 唤醒——否则线程永远阻塞，`jthread` 析构时 join 会死等。
- **回调要轻量**：HFT 关机要求快速——`stop_callback` 中不要做重操作（如刷盘），只做通知和设置标志，重操作留给工作线程的退出清理。
- **注册时机**：HFT 线程启动后立即注册 `stop_callback`——确保关机请求不会因为"回调还没注册"而丢失。

---

## 自测题

1. `stop_callback` 注册的回调在什么时候执行？在哪个线程执行？
2. 如果注册回调时 `stop` 已经被请求，回调会怎样？
3. `stop_callback` 对象析构时如果回调还没执行，会发生什么？
4. 如何用 `stop_callback` 唤醒在 `condition_variable` 上阻塞的工作线程？
5. HFT 系统关机时，`stop_callback` 中应该做什么？不应该做什么？

---

## 参考与延伸

- 下一节：[9.5 协程简介](05-coroutine.md)
- 上一节：[9.3 jthread 与协作式中断](03-jthread.md)
- 回到：[第 9 章](README.md)
