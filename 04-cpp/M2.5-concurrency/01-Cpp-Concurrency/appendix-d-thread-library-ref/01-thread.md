# D.1 thread 线程管理

> 附录 D C++ 线程库参考 · 上一章：[C.6 完整流程](../appendix-c-atm-example/06-full-flow.md) · 下一节：[D.2 mutex 互斥锁](02-mutex.md)

## 这节讲什么

`<thread>` 头文件提供线程管理设施。本节是速查参考——`std::thread`/`jthread` 的接口、`this_thread` 命名空间、以及线程生命周期管理。

---

## 核心规则（代码+表格）

### `std::thread` 接口

| 接口 | 说明 |
|------|------|
| `thread(f, args...)` | 创建线程，执行 f(args...) |
| `~thread()` | 析构：未 join/detach → `std::terminate` |
| `join()` | 等待线程结束 |
| `detach()` | 分离（后台运行） |
| `joinable()` | 是否可 join（有关联线程） |
| `get_id()` | 线程 ID |
| `native_handle()` | 原生句柄（pthread_t / HANDLE） |
| `hardware_concurrency()` | 硬件并发数（静态） |

```cpp
// 创建
std::thread t1([]{ work(); });
std::thread t2(func, arg1, arg2);

// 等待
t1.join();
t2.join();

// 移动（不可拷贝）
std::thread t3 = std::move(t1);  // t1 变 not-a-thread

// 放入容器
std::vector<std::thread> threads;
threads.emplace_back([]{ work1(); });
threads.emplace_back([]{ work2(); });
for (auto& t : threads) t.join();
```

### `std::jthread`（C++20）

| 接口 | 说明 |
|------|------|
| `jthread(f, args...)` | 创建，自动接收 stop_token |
| `~jthread()` | 析构：自动 request_stop + join |
| `request_stop()` | 请求协作式中断 |
| `get_stop_source()` | 获取 stop_source |
| `get_stop_token()` | 获取 stop_token |

```cpp
std::jthread t([](std::stop_token st){
    while (!st.stop_requested()) {
        work();
    }
});
// 析构自动 request_stop + join
```

### `std::this_thread` 命名空间

| 函数 | 说明 |
|------|------|
| `get_id()` | 当前线程 ID |
| `sleep_for(duration)` | 睡眠时长 |
| `sleep_until(time_point)` | 睡到时间点 |
| `yield()` | 让出 CPU（提示调度器） |

```cpp
using namespace std::chrono_literals;

std::this_thread::sleep_for(100ms);           // 睡 100 毫秒
std::this_thread::sleep_for(1s);              // 睡 1 秒
std::this_thread::sleep_until(steady_clock::now() + 500ms);

std::this_thread::yield();  // 让出 CPU（忙等时用）

auto id = std::this_thread::get_id();  // 当前线程 ID
```

### 线程 ID

```cpp
std::thread::id main_id = std::this_thread::get_id();
std::cout << "main thread id: " << main_id << "\n";

std::thread t([]{
    std::cout << "worker id: " << std::this_thread::get_id() << "\n";
});
t.join();

// ID 可比较（==, !=）和哈希（std::hash<thread::id>）
if (std::this_thread::get_id() == main_id) {
    std::cout << "in main thread\n";
}
```

### `native_handle()` 用于绑核

```cpp
#include <pthread.h>
#include <sched.h>

std::thread t([]{ work(); });

// 绑核（Linux）
cpu_set_t cpuset;
CPU_ZERO(&cpuset);
CPU_SET(2, &cpuset);  // 绑核 2
pthread_setaffinity_np(t.native_handle(), sizeof(cpuset), &cpuset);

t.join();
```

---

## 新手要点（和 C 的区别）

- **C 用 `pthread_create`/`pthread_join`**：C++ 的 `std::thread` 是面向对象封装——构造即启动，析构要 join。C 程序员转型时要改掉"忘记 join"的习惯——`std::thread` 析构未 join 会 `terminate`。
- **`jthread` 是 C++20 的新选择**：C 程序员如果可以，优先用 `jthread`——析构自动 join + 支持协作式中断。`std::thread` 更底层，容易出错。
- **`native_handle()` 是 C 程序员熟悉的**：C 程序员用 `pthread_t` 做底层操作（绑核、优先级）——C++ 的 `native_handle()` 返回 `pthread_t`（Linux），可以继续用 pthread API。
- **`hardware_concurrency()` 是提示而非保证**：C 程序员可能觉得"返回核数"——但它可能返回 0（无法确定），不应作为硬依赖。

---

## HFT 关联

- **HFT 用 `std::thread` + `native_handle()` 绑核**：HFT 必须绑核消除调度抖动——`pthread_setaffinity_np(t.native_handle(), ...)` 是标准做法。
- **HFT 不用 `jthread` 的 stop_token**：HFT 热路径不检查 `stop_requested()`（有分支开销）。关机用自定义信号 + 批次间检查。
- **`yield()` 在 HFT 中的用途**：HFT 的忙等循环（如等 SPSC 队列）用 `yield()` 让出超线程的兄弟核——避免一个核被忙等占满。
- **`sleep_for` 不用于 HFT 热路径**：`sleep_for` 有调度器参与，延迟不确定。HFT 用忙等（`while` 循环 + `yield`）而非睡眠。

---

## 自测题

1. `std::thread` 析构时如果未 join 也未 detach，会发生什么？
2. `std::jthread`（C++20）相比 `std::thread` 有什么优势？
3. `this_thread::yield()` 的作用是什么？什么时候用？
4. 如何用 `native_handle()` 实现线程绑核？
5. 为什么 HFT 热路径用忙等而非 `sleep_for`？

<details>
<summary>参考答案</summary>

1. 若 `std::thread` 对象析构时仍为 joinable，标准要求调用 `std::terminate()`，不会自动 join 或 detach。
   应在所有退出路径显式处理，或使用 RAII 包装/`std::jthread`。
2. `jthread` 析构时会请求停止并 join，还可把关联的 `stop_token` 注入兼容的线程函数。
   这降低异常路径遗漏 join 的风险，并提供标准协作取消机制。
3. `yield()` 向调度器提示当前线程愿意让出剩余时间片，但是否切换及何时再次运行都不保证。
   可在短暂自旋的退避策略中使用，不能作为同步原语或可靠等待机制。
4. `native_handle()` 取得实现定义的原生句柄，再调用平台 API 设置 CPU affinity，例如 Linux 的 `pthread_setaffinity_np`。
   代码不可移植，必须检查返回值，并结合 NUMA、隔离核和线程启动时机验证实际绑定结果。
5. `sleep_for` 可能进入内核并经历定时器粒度、唤醒和重新调度，尾延迟通常不可预测。
   忙等可持续轮询并快速响应，但独占 CPU、耗能且会干扰同核线程，只适合专用核和极短等待。

</details>

---

## 参考与延伸

- 下一节：[D.2 mutex 互斥锁](02-mutex.md)
- 上一章：[C.6 完整流程](../appendix-c-atm-example/06-full-flow.md)
- 回到：[附录 D](README.md)
