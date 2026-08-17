# 9.1 线程池基础

> 第 9 章 高级线程管理 · 上一章：[8.6 负载均衡](../ch08-designing-concurrent-code/06-load-balancing.md) · 下一节：[9.2 Work-Stealing 调度](02-work-stealing.md)

## 这节讲什么

手写 `std::thread` 适合简单场景，复杂系统需要线程池——预创建一组线程，反复接收任务，避免线程创建/销毁的开销。本节讲线程池的基本设计、任务队列、提交/等待接口，以及它的局限性。

---

## 核心规则（代码+表格）

### 基础线程池实现

```cpp
class thread_pool {
    std::vector<std::thread> workers;
    std::queue<std::function<void()>> tasks;
    std::mutex m;
    std::condition_variable cv;
    bool stop = false;

public:
    explicit thread_pool(size_t n) {
        for (size_t i = 0; i < n; ++i) {
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
                    task();  // 锁外执行
                }
            });
        }
    }

    ~thread_pool() {
        { std::lock_guard<std::mutex> lk(m); stop = true; }
        cv.notify_all();
        for (auto& t : workers) t.join();
    }

    // 提交任务
    template <typename F>
    void submit(F&& f) {
        {
            std::lock_guard<std::mutex> lk(m);
            tasks.emplace(std::forward<F>(f));
        }
        cv.notify_one();
    }

    // 提交并获取 future（C++11 版本）
    template <typename F>
    auto submit_with_future(F&& f) -> std::future<decltype(f())> {
        using R = decltype(f());
        auto task = std::make_shared<std::packaged_task<R()>>(std::forward<F>(f));
        auto fut = task->get_future();
        {
            std::lock_guard<std::mutex> lk(m);
            tasks.emplace([task]{ (*task)(); });
        }
        cv.notify_one();
        return fut;
    }
};
```

### 线程池的核心组件

| 组件 | 作用 |
|------|------|
| 工作线程数组 | 预创建的固定数量线程 |
| 任务队列 | `std::queue<std::function<void()>>` |
| mutex + condition_variable | 保护队列 + 唤醒空闲线程 |
| stop 标志 | 析构时通知线程退出 |

### `packaged_task` 桥接 future

```cpp
// 问题：std::function<void()> 无法返回值/异常
// 解决：用 packaged_task 包装，future 获取结果

template <typename R>
auto wrap(std::packaged_task<R()>& task) {
    return [&task]{ task(); };  // lambda 捕获 task 引用
}
// 注意生命周期：task 必须存活到执行完
// 更安全：用 shared_ptr 延长生命周期（见上面 submit_with_future）
```

### 线程池的局限

| 局限 | 说明 |
|------|------|
| 固定线程数 | 任务耗时不均时负载不均（需 work-stealing） |
| 单任务队列 | 高并发提交时队列竞争（需分段队列） |
| 无优先级 | 所有任务 FIFO（需优先级队列） |
| 无任务取消 | 提交后无法撤销 |
| 无异常传递 | 需要靠 future 显式 get |

---

## 新手要点（和 C 的区别）

- **C 里通常手写线程池或用第三方库**：C 标准库没有线程池（C11 只有 `thrd_create`）。C 程序员要么自己写（pthread + 条件变量），要么用 POSIX 库。C++ 也没有标准线程池（直到 C++26 可能引入 `std::execution`），但 `std::function` + `packaged_task` 让实现比 C 简洁很多。
- **`std::function<void()>` 是类型擦除的关键**：C 里要用函数指针 + `void*` 参数，类型不安全。C++ 的 `std::function` 可以包装任何可调用对象（lambda、函数指针、仿函数），统一成 `void()` 类型存入队列。这是 C++ 线程池比 C 线程池优雅的根本原因。
- **`packaged_task` 是 C 程序员陌生的新工具**：C 里要手动传 `pthread_result` 或用全局变量传递返回值。C++ 的 `packaged_task` + `future` 是标准化的异步结果传递机制——这是 C++ 并发的高级工具。
- **`condition_variable::wait(lk, pred)` 的谓词形式**：C 里要手动循环 `pthread_cond_wait` + 检查条件（防虚假唤醒）。C++ 的谓词形式自动处理虚假唤醒——更简洁、更不易出错。

---

## HFT 关联

- **HFT 通常不用通用线程池**：通用线程池的任务队列竞争和调度不确定性不适合 HFT 的低延迟需求。HFT 通常用"固定流水线 + SPSC 队列"——每线程固定角色，无任务调度。
- **线程池用于盘后/非热路径**：盘后批处理、日志分析、回测可以用线程池——这些场景不追求纳秒级延迟，通用线程池够用。
- **`std::function` 的堆分配代价**：`std::function` 在捕获大对象时可能堆分配——HFT 热路径要避免。可以用固定大小的任务结构 + 函数指针，或 C++23 的 `std::move_only_function`。
- **线程数 = 核数**：HFT 线程池的线程数应等于物理核数（或更少），每线程绑核——避免 OS 调度器迁移线程。

---

## 自测题

1. 线程池的核心组件有哪些？为什么用 `std::function<void()>` 存任务？
2. `submit_with_future` 如何用 `packaged_task` 桥接 future？为什么要用 `shared_ptr`？
3. `condition_variable::wait(lk, pred)` 的谓词形式解决了什么问题？
4. 线程池有哪些局限？为什么 HFT 通常不用通用线程池？
5. `std::function` 在 HFT 热路径中有什么代价？

---

## 参考与延伸

- 下一节：[9.2 Work-Stealing 调度](02-work-stealing.md)
- 上一章：[8.6 负载均衡](../ch08-designing-concurrent-code/06-load-balancing.md)
- 回到：[第 9 章](README.md)
