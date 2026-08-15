# 第 7 章 并发 API

**The Concurrency API** — Items 35–40

## 本章讲什么

C++11 把线程、原子操作、future 首次标准化。但标准并发 API 有不少"看着对、其实错"的坑：`std::async` 的默认启动策略、`std::thread` 的异常安全与析构行为、`volatile` 与 `std::atomic` 的本质区别。本章是 HFT 并发的入门必修——更深的内容在 02-Cpp-Concurrency（C++ Concurrency in Action）。

---

## 各 Item 要点

### Item 35：优先 `std::async` 而非 `std::thread`

`std::thread` 是"手动管理线程"，`std::async` 是"声明并发任务，让运行时管线程"。

```cpp
auto fut = std::async([]{ return 42; });  // 异步执行，future 取结果
```

`async` 的优势：返回 `future`，异常会通过 future 传播（`thread` 里抛异常直接 `terminate`）；运行时可决定是否真正并发（延迟到 `get()` 才执行，即 `std::launch::deferred`）。

### Item 36：明确指定启动策略

`async` 默认策略是 `async | deferred`——运行时可能**延迟到 `get()` 才同步执行**，这违背"并发"初衷，且导致 `.get()` 阻塞。要真正异步，显式指定：

```cpp
auto fut = std::async(std::launch::async, []{ ... });
```

`launch::async` 强制新线程或线程池执行；`launch::deferred` 强制延迟。默认的"或"策略是最容易被忽略的陷阱——你以为是异步，实际是同步阻塞。

### Item 37：让 `std::thread` 在所有路径都不可联结（joinable）

`std::thread` 析构时若仍 `joinable`（既未 `join` 也未 `detach`）→ **`std::terminate`**。RAII 保证：

```cpp
class ThreadGuard {
    std::thread t;
public:
    ~ThreadGuard() { if (t.joinable()) t.join(); }
};
```

或在异常路径用"先 `join` 后业务逻辑"的顺序，确保异常发生时线程已安全收尾。

### Item 38：理解线程句柄的析构与联结行为

| 句柄类型 | 析构行为 |
|----------|----------|
| `std::thread` | joinable 则 `terminate` |
| `std::future` | 阻塞等待（共享状态未就绪时）——析构会等任务完成 |
| `std::shared_future` | 不阻塞（多个 shared_future 共享状态） |

`future` 析构会阻塞这一行为反直觉——它让你"忘了 get 也能拿到结果"，但也可能让程序在意外处卡住。

### Item 39：单次事件用 `std::future` + `std::promise` / `std::experimental::latch` / condition_variable

经典模式：一个线程等另一个线程"完成一次初始化"。用 `std::promise<void>` 发信号 + `future.get()` 阻塞等待，比手写 condition_variable + flag 更不易错（无虚假唤醒、无锁泄漏）。C++20 的 `std::latch` / `std::barrier` 是这类同步原语的标准化封装。

### Item 40：`std::atomic` 用于并发，`volatile` 用于特殊内存——别混用

二者**完全不同**：

| | `std::atomic` | `volatile` |
|---|---------------|------------|
| 目的 | 多线程数据竞争 | 告诉编译器"别优化此内存访问"（MMIO） |
| 可见性 | 保证（内存屏障） | **不保证**跨线程可见性 |
| 原子性 | 保证 | 不保证 |
| 指令重排 | 阻止相关重排 | **不阻止** |

`volatile` 在 C++ 里**不是**线程同步工具——它只防编译器把读写优化掉（如硬件寄存器、mmap 内存）。多线程共享变量必须用 `std::atomic` 或 mutex。这是 C++ 程序员最普遍的误解之一。

---

## HFT 关联

- **`std::launch::async` 必显式**：HFT 异步任务（如批量风控检查）若用默认 `async` 策略，可能被延迟到 `get()` 同步执行，热路径意外阻塞。务必 `launch::async`。
- **`std::atomic` 而非 `volatile`**：行情计数器、序号用 `std::atomic<uint64_t>`，保证跨核可见性 + 原子性。`volatile` 只用于 mmap 的硬件寄存器映射（DPDK PMD 配置寄存器）。混用是经典数据竞争来源。
- **`std::thread` 析构 terminate**：HFT 守护进程里 `std::thread` 析构时若仍 joinable 会直接 `terminate` 拉崩整个进程——用 RAII 守卫或显式 `join`/`detach`。
- **future 阻塞析构**：异步风控任务用 `future`，但要确保不在热路径析构 future（析构会等任务完成）——把 future 存到后台线程或用 `shared_future`。

---

## 自测题

1. `std::async` 的默认启动策略是什么？为什么它可能导致"以为是异步实际是同步"？
2. `std::thread` 析构时仍 joinable 会发生什么？如何用 RAII 规避？
3. `std::future` 析构会做什么？这与 `std::thread` 的析构行为有何不同？
4. `std::atomic` 和 `volatile` 的本质区别是什么？为什么 `volatile` 不能用于线程同步？
5. 单次事件同步用 `promise<void>` + `future` 相比 condition_variable + flag 有什么优势？



## 代码自测

### Q1: async 默认策略陷阱

```cpp
auto fut = std::async([]{ 
    std::this_thread::sleep_for(std::chrono::seconds(1));
    return 42; 
});
// fut.get() 是立即返回还是等待 1 秒？
```

> `fut.get()` 会阻塞吗？为什么？

<details>
<summary>答案与复习指引</summary>

**会等待约 1 秒。** `async` 默认策略是 `std::launch::async | std::launch::deferred`——运行时可以选择异步执行或延迟到 `get()` 才同步执行。如果选了 `deferred`，`get()` 会阻塞直到任务同步完成。

**修复：** `std::async(std::launch::async, []{ ... });`——强制新线程/线程池执行，保证异步。

**HFT 教训：** 以为异步实际同步阻塞，热路径意外卡住。务必显式指定 `launch::async`。

**复习：** → [Item 36：明确指定启动策略](item36-launch-policy.md)
</details>

### Q2: volatile 不是 atomic

```cpp
volatile int flag = 0;
// 线程 1: flag = 1;
// 线程 2: while (flag == 0) ;
```

> 这段代码线程安全吗？`volatile` 能保证什么？

<details>
<summary>答案与复习指引</summary>

**不是线程安全的——数据竞争 UB。**

**`volatile` 保证：** 编译器不优化掉对该变量的读写（用于 MMIO/硬件寄存器）。
**`volatile` 不保证：** ①原子性（读/写可能被撕裂）②跨线程可见性（无内存屏障）③指令重排限制。

**正确做法：** `std::atomic<int> flag{0};`——保证原子性 + 可见性 + 重排限制。

**这是 C++ 最普遍的误解之一：** `volatile` 在 C++ 里不是线程同步工具。

**复习：** → Item 40：std::atomic 用于并发，volatile 用于特殊内存
</details>
