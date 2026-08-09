# 第 14 章 协程

**Coroutines**

## 本章讲什么

C++20 终于引入**协程**——函数可以挂起和恢复，让异步代码写起来像同步。本章是协程入门：`co_await`/`co_yield`/`co_return`、协程机制、与传统回调的对比。

## 要点

### 什么是协程

协程是**可挂起和恢复**的函数。普通函数从头跑到尾，协程可以在中间暂停（`co_await`/`co_yield`），让出执行权，稍后从暂停点恢复。

```
普通函数：开始 → 执行 → 结束（一次性）
协程：开始 → 执行 → 挂起 → [其他代码运行] → 恢复 → 执行 → 结束
```

### 三个关键字

```cpp
// co_yield：挂起并产出一个值（生成器）
Generator<int> gen() {
    for (int i = 0; ; ++i) {
        co_yield i;   // 产出 i，挂起，恢复时继续
    }
}

// co_return：结束协程并返回值
Task<int> compute() {
    co_return 42;
}

// co_await：等待一个异步操作完成
Task<void> async_work() {
    auto result = co_await async_read();
    co_await async_write(result);
}
```

### 生成器示例

```cpp
// 简化的生成器（实际要实现 promise_type）
Generator<int> naturals() {
    int i = 1;
    while (true) {
        co_yield i++;
    }
}

auto g = naturals();
std::cout << g.next();  // 1
std::cout << g.next();  // 2
std::cout << g.next();  // 3
// 惰性生成，无限序列不耗内存
```

### 协程的机制

协程的挂起/恢复靠编译器生成的**状态机**：
1. 协程开始：在堆上分配**协程帧**（保存局部变量、暂停点）。
2. 挂起：保存状态到协程帧，返回给调用方。
3. 恢复：从协程帧恢复状态，从暂停点继续。
4. 结束：释放协程帧。

协程帧的生命周期由 `promise_type` 管理——这是 C++20 协程的底层机制，使用者通常不直接写。

### `promise_type` 与协程返回类型

C++20 没有现成的 `Task`/`Generator` 类型——标准库只给了机制（`co_await`/`co_yield`/`co_return`），具体返回类型要自己实现或用第三方库（`cppcoro`、`asio::awaitable`）。

```cpp
// 自定义协程返回类型要实现 promise_type
struct Generator {
    struct promise_type {
        int current;
        Generator get_return_object() { return Generator{handle_type::from_promise(*this)}; }
        std::suspend_always initial_suspend() { return {}; }
        std::suspend_always final_suspend() noexcept { return {}; }
        std::suspend_always yield_value(int v) { current = v; return {}; }
        void return_void() {}
        void unhandled_exception() { std::terminate(); }
    };
    // ...
};
```

### 协程 vs 回调 vs future

| 方案 | 写法 | 问题 |
|------|------|------|
| 回调 | `async_read(cb)` | 回调地狱、控制流分散 |
| future | `f = async(); f.get()` | 阻塞、嵌套难 |
| 协程 | `co_await async_read()` | 同步风格写异步、可组合 |

## HFT 关联

- **异步行情订阅**：`co_await subscribe("AAPL")` 替代回调，代码线性可读。
- **批量异步 IO**：`co_await read_batch()` + `co_await process()` 流水线式异步，无回调嵌套。
- **热路径仍用同步**：协程挂起/恢复有状态机开销（保存/恢复协程帧），纳秒级热路径仍用同步 + SPSC 队列。
- **管理通道适用**：策略管理、配置拉取、监控上报用协程，异步 IO 代码清晰。
- **C++20 协程库不成熟**：标准库没给 `Task`/`Generator`，要用 `asio`/`cppcoro`——HFT 慎重选库。
- **堆分配注意**：协程帧默认堆分配，热路径要 RAII（`suspend_always` + 取消分配）或用池化。

## 自测题

1. 协程和普通函数的核心区别是什么？
2. `co_await`、`co_yield`、`co_return` 分别做什么？
3. 协程的挂起/恢复机制是什么？协程帧的作用？
4. C++20 协程为什么"只有机制没有库"？要怎么用？
5. HFT 热路径为什么仍用同步？协程适合什么场景？
