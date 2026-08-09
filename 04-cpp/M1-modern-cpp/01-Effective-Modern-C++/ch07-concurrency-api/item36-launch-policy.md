# Item 36：明确指定启动策略

> 第 7 章 · Item 36 · 上一节：[Item 35 async vs thread](item35-async-vs-thread.md)

## 为什么要学这个（先建立直觉）

C 程序员用 `pthread_create` 时，线程**一定**立即创建并执行：

```c
pthread_t tid;
pthread_create(&tid, NULL, worker, NULL);  // 立即在新线程执行
// worker 一定已经开始运行
pthread_join(tid, NULL);  // 等待完成
```

C++ 的 `std::async` 默认策略是 `async | deferred`——运行时可能选择延迟到 `get()` 才**同步**执行。这意味着你以为在异步执行，实际可能同步阻塞：

```cpp
auto fut = std::async([]{ return compute(); });
// 可能已经开始异步执行
// 也可能什么都没做——延迟到 get() 才同步执行
auto result = fut.get();  // 如果是 deferred → 在这里同步执行！
```

这是 C++ 并发 API 最隐蔽的陷阱——你以为异步，实际同步。

---

## 这节讲什么

`async` 默认策略是 `async | deferred`——运行时可能延迟到 `get()` 才同步执行，这违背"并发"初衷。

---

## 核心问题

### 默认策略的陷阱

```cpp
auto fut = std::async([]{ 
    heavy_computation(); 
    return 42; 
});
// 默认策略：launch::async | launch::deferred
// 运行时可能：
//   1. 立即在新线程执行（async）
//   2. 延迟到 get() 才在当前线程同步执行（deferred）
// 你无法控制是哪种！
```

### 显式指定策略

```cpp
// 强制异步：一定在新线程执行
auto fut = std::async(std::launch::async, []{ ... });
// 如果系统资源不足 → 抛 std::system_error

// 强制延迟：到 get() 才在当前线程执行
auto fut2 = std::async(std::launch::deferred, []{ ... });
auto result = fut2.get();  // 在当前线程同步执行
// 适合 lazy evaluation 场景
```

| 策略 | 行为 | 适用场景 |
|------|------|---------|
| `launch::async` | 强制新线程/线程池执行 | 并发任务 |
| `launch::deferred` | 延迟到 `get()`/`wait()` 同步执行 | 延迟计算 |
| 默认（`async \| deferred`） | 运行时决定 | 不推荐——行为不确定 |

### 检测 deferred

```cpp
auto fut = std::async([]{ ... });
// 检测是否 deferred
if (fut.wait_for(std::chrono::seconds(0)) == std::future_status::deferred) {
    // 是 deferred——get() 会同步执行
    auto result = fut.get();  // 在当前线程执行
} else {
    // 是 async——已经在异步执行
    auto result = fut.get();  // 等待异步完成
}
```

---

## 常见错误（新手踩坑）

**错误 1：用默认策略以为一定异步**
```cpp
auto fut = std::async([]{ heavy_work(); });
// 可能 deferred → get() 时同步阻塞 → 热路径卡住
do_something_else();  // 以为 heavy_work 在后台跑
fut.get();  // 如果 deferred → 这里才执行 heavy_work！
```
**修正：** 显式 `std::launch::async`。

**错误 2：wait_for(0) 检测 deferred 的用法搞错**
```cpp
auto fut = std::async([]{ ... });
if (fut.wait_for(std::chrono::seconds(0)) == std::future_status::ready) {
    // ready 表示已完成——但 deferred 也返回 deferred 不是 ready
}
```
**修正：** 检查 `== std::future_status::deferred`。

**错误 3：launch::async 资源不足抛异常未处理**
```cpp
auto fut = std::async(std::launch::async, []{ ... });
// 如果线程数超限 → 抛 std::system_error
```
**修正：** `try-catch` 或确保系统资源充足。

---

## 新手要点（和 C 的区别）

| 维度 | C 怎么做 | C++ 怎么做 | 为什么 |
|------|---------|-----------|--------|
| 线程创建 | `pthread_create` 立即执行 | `async` 默认可能延迟 | C++ 给运行时灵活性 |
| 策略控制 | 不适用 | `launch::async`/`deferred` | C++ 标准库 |
| 行为确定性 | 一定异步 | 默认不确定 | 需要显式指定 |

**一句话总结：** C 程序员记住——C++ 的 `async` 默认策略可能延迟到 `get()` 才同步执行。**永远显式写 `launch::async`**。

---

## HFT 关联

- **热路径意外阻塞**：HFT 异步任务若用默认 `async` 策略，可能被延迟到 `get()` 同步执行，热路径意外阻塞。务必 `launch::async`。
- **延迟计算**：配置加载用 `launch::deferred`——用到时才计算，启动快。
- **线程池**：`launch::async` 可能由运行时用线程池实现——避免频繁创建线程的开销。

---

## 自测题

1. `std::async` 的默认启动策略是什么？为什么可能导致"以为是异步实际是同步"？
2. `launch::async` 和 `launch::deferred` 分别是什么行为？
3. 为什么 HFT 必须显式指定 `launch::async`？
4. 如何检测 `async` 返回的 future 是否被 deferred？
5. 下面代码有什么问题？
```cpp
auto fut = std::async([]{ return heavy_work(); });
process_other();
int result = fut.get();
```

---

## 参考与延伸

- 下一节：[Item 37 thread joinable](item37-thread-joinable.md)
- 回到：[第 7 章](README.md)
