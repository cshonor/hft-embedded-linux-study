# 9.5 协程简介

> 第 9 章 · 上一节：[9.4 stop_callback](04-stop-callback.md) · 下一章：[10.1 三种执行策略](../ch10-parallel-algorithms/01-execution-policies.md)

## 这节讲什么

C++20 引入协程（coroutine）——函数可以暂停（`co_await`）和恢复，而不阻塞线程。本节简要介绍协程的基本概念、`co_await`/`co_return`/`co_yield` 三个关键字、以及协程在异步 IO 中的应用。注意：C++20 只提供协程的语言设施，库设施（如 `std::task`）要到 C++23/26。

---

## 核心规则（代码+表格）

### 协程三关键字

```cpp
// co_await：暂停当前协程，等待异步操作完成
task<int> fetch_data() {
    auto data = co_await async_read();  // 暂停，不阻塞线程
    co_return process(data);            // 协程返回值
}

// co_yield：产出一个值，暂停（生成器）
generator<int> counter() {
    int i = 0;
    while (true) {
        co_yield i++;  // 产出 i，暂停；恢复时继续
    }
}

// co_return：协程结束，返回最终值
task<void> do_work() {
    co_await step1();
    co_await step2();
    co_return;  // 结束
}
```

### 协程 vs 线程 vs 回调

| 方式 | 编程模型 | 阻塞 | 开销 |
|------|----------|------|------|
| 回调 | 异步、碎片化 | 不阻塞线程 | 最小 |
| 协程 | 异步、线性写法 | 不阻塞线程 | 小（栈帧在堆） |
| 线程 | 同步、阻塞 | 阻塞线程 | 大（栈 1-8MB） |

### 协程的暂停与恢复

```cpp
// 伪代码：协程的内部机制
task<int> example() {
    int x = co_await async_op();  // 暂停点 1
    int y = co_await async_op2(); // 暂停点 2
    co_return x + y;
}
// 调用 example() → 创建协程帧（堆上）→ 执行到暂停点 1 → 返回 task
// async_op 完成后 → 恢复协程 → 继续到暂停点 2 → 再次暂停
// async_op2 完成后 → 恢复 → co_return → 销毁协程帧
```

### 异步 IO 的协程写法

```cpp
// 传统回调地狱
void fetch_and_process() {
    async_connect([](socket s){
        async_read(s, [](buffer b){
            async_process(b, [](result r){
                async_write(s, r, [](){
                    std::cout << "done\n";
                });
            });
        });
    });
}

// 协程：线性写法，无嵌套
task<void> fetch_and_process_coro() {
    auto s = co_await async_connect();
    auto b = co_await async_read(s);
    auto r = co_await async_process(b);
    co_await async_write(s, r);
    std::cout << "done\n";
}
```

### C++20 协程的局限

| 局限 | 说明 |
|------|------|
| 只有语言设施 | `co_await` 等关键字可用，但没有 `std::task`、`std::generator` |
| 需要自己实现 promise_type | 或用第三方库（cppcoro、folly、ASIO） |
| 调试困难 | 调用栈不连续（协程帧在堆上） |
| 学习曲线陡 | promise_type / awaiter 机制复杂 |

---

## 新手要点（和 C 的区别）

- **C 没有协程**：C 程序员做异步 IO 通常用回调（libuv、libevent）或线程。协程的"线性写法但异步执行"是 C 程序员陌生的新模式——代码看起来像同步阻塞，但实际不阻塞线程。
- **协程 ≠ 线程**：C 程序员可能把协程理解为"轻量级线程"——不完全对。协程是**协作式**调度（自己 `co_await` 时才让出），线程是**抢占式**（OS 调度）。协程切换在用户态，无内核参与。
- **`co_await` 的本质**：C 程序员可以理解为"回调的语法糖"——`co_await expr` 等价于"注册回调，回调里继续执行后续代码"。但写法是线性的，不是嵌套的。
- **C++20 协程不完整**：C 程序员可能期望"开箱即用"——但 C++20 只提供语言机制，要自己实现 `promise_type` 或用第三方库。直到 C++23 的 `std::task`（部分）和 C++26 才有标准库设施。实际使用通常依赖 ASIO 的协程支持。

---

## HFT 关联

- **HFT 热路径不用协程**：协程的暂停/恢复有开销（堆帧分配、间接调用），不适合纳秒级热路径。HFT 热路径用固定流水线 + SPSC 队列。
- **协程用于 HFT 的管理面**：如网关连接管理、配置加载、监控上报——这些异步 IO 场景用协程比回调清晰。ASIO 的协程支持（`co_spawn` + `awaitable`）是 C++ 网络编程的现代写法。
- **ASIO + 协程**：HFT 系统的 TCP 网关（接收交易所回报）用 ASIO 协程——线性写法处理连接、读取、解析、回复，比回调嵌套清晰得多。
- **协程的栈帧在堆上**：HFT 如果用协程，要注意堆分配——可以用自定义 allocator 的 promise_type 做栈上分配。但热路径通常避免协程。

---

## 自测题

1. 协程和线程有什么本质区别？调度方式有何不同？
2. `co_await`、`co_yield`、`co_return` 各自的作用是什么？
3. 为什么说协程是"回调的语法糖"？它解决了什么问题？
4. C++20 协程有什么局限？为什么实际使用通常依赖第三方库？
5. HFT 系统的哪些部分适合用协程？热路径适合吗？

---

## 参考与延伸

- 下一章：[10.1 三种执行策略](../ch10-parallel-algorithms/01-execution-policies.md)
- 上一节：[9.4 stop_callback](04-stop-callback.md)
- 回到：[第 9 章](README.md)
