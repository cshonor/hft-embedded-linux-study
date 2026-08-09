# 1.1 并发 vs 并行

> 第 1 章 你好，并发 · 下一节：[1.2 并发动机](02-motivation.md)

## 这节讲什么

并发（concurrency）和并行（parallelism）常被混用，但它们是不同层次的概念。理解区别才能正确选择并发设计策略——并发是"如何组织多个任务"，并行是"如何让它们真正同时跑"。

## 为什么要学这个（先建立直觉）

先看 C 程序员熟悉的场景：一个单线程程序处理网络数据。

```c
// C：单线程，串行处理
while (1) {
    read_socket(buf);      // 阻塞等数据
    process(buf);          // 处理数据
    write_socket(result);  // 发回结果
}
// 问题：read 阻塞时，CPU 空闲，后面的 process 也被卡住
```

并发设计把"等数据"和"处理数据"拆开，让它们交替进行：

```c
// C：用 select/poll 做并发（单线程）
while (1) {
    poll(fds, ...);        // 同时监听多个 fd
    if (fd1_ready) read(fd1, buf1);
    if (fd2_ready) read(fd2, buf2);
    process(buf1); process(buf2);  // 交替处理
}
// 这是并发（一个线程管理多个任务），但不是并行（仍然单核）
```

C++ 进一步提供了 `std::thread`，让你可以真正并行：

```cpp
// C++：多线程并行
std::thread t1(process_loop, fd1);  // 核心 1 处理 fd1
std::thread t2(process_loop, fd2);  // 核心 2 处 fd2
t1.join(); t2.join();
```

## 核心区别

- **并发（concurrency）**：逻辑上的"同时"——单核时间片切换也能并发。并发是**设计**（管理多个任务）。
- **并行（parallelism）**：物理上的"同时"——多核真正同时执行。并行是**执行**（同时跑）。

```
单核并发：[A][B][A][B][A][B]  时间片切换（逻辑同时）
多核并行：核心1: [A A A]
          核心2: [B B B]       真正同时（物理同时）
并发+并行：核心1: [A A A]      ← 最理想：既拆分了任务，又能同时跑
          核心2: [B B B]
```

| 维度 | 并发 | 并行 |
|------|------|------|
| 本质 | 任务组织方式 | 任务执行方式 |
| 需要多核？ | 不需要 | 需要 |
| 目标 | 独立管理多个任务 | 同时加速 |
| C 对应 | select/poll/epoll | pthread/fork |
| C++ 对应 | std::async/std::thread | std::thread + 多核 |

并发不一定需要多核——单核也能并发（时间片切换）。并行一定需要多核。但最好的方案是两者结合：先做好并发设计（拆分独立任务），再利用多核并行执行。

## 常见错误（新手踩坑）

### 错误 1：以为多线程一定更快

```cpp
// 错误：多线程访问共享数据，锁竞争反而更慢
std::mutex mtx;
int counter = 0;
void worker() {
    for (int i = 0; i < 1000000; i++) {
        std::lock_guard<std::mutex> lk(mtx);  // 每次都锁！
        counter++;
    }
}
// 4 线程跑可能比单线程还慢——锁竞争 + cache line bouncing
```

**修复**：减少共享，或用 `std::atomic`，或每个线程独立计数最后汇总。

### 错误 2：把并发和并行混为一谈

```
// 混淆：说"我的程序是并行的"但实际只是单线程 + async
std::async(std::launch::async, task1);  // 如果只有 1 核，这只是并发不是并行
```

**记住**：并行需要硬件支持（多核），并发是设计决策。

### 错误 3：单核假设导致并发 bug 被隐藏

```cpp
// 在单核机器上测试通过，多核上崩溃
bool ready = false;
int data = 0;
// 线程 1
data = 42;
ready = true;
// 线程 2
while (!ready) {}
std::cout << data;  // 单核可能打印 42，多核可能打印 0（数据竞争 + 重排）
```

**修复**：用 `std::atomic<bool>` 配合正确的内存序，或多核环境测试。

## 和 C 的区别

| 特性 | C (pthread) | C++ (std::thread) |
|------|-------------|-------------------|
| 创建线程 | `pthread_create(&t, NULL, func, arg)` | `std::thread t(func, arg)` |
| 等待线程 | `pthread_join(t, NULL)` | `t.join()` |
| 分离线程 | `pthread_detach(t)` | `t.detach()` |
| 线程句柄 | `pthread_t`（裸句柄） | `std::thread`（RAII，析构检查） |
| 传参 | `void*` 手动转型 | 模板类型推导，支持引用/移动 |
| 错误处理 | 返回错误码 | 抛 `std::system_error` |

## HFT 关联

- **多线程 + 绑核是 HFT 标配**：行情线程绑核 0、策略线程绑核 1、风控线程绑核 2，避免上下文切换延迟。
- **并发设计的首要目标是确定性**：不是"跑得多快"而是"每次都一样快"——数据竞争导致的非确定延迟比慢更危险。
- **单线程 + 协程也有用**：某些 HFT 场景用单线程 + 事件循环（类似 epoll），避免锁开销，保证顺序确定性。

## 代码自测

### Q1: 下列代码是并发、并行、还是两者都是？

```cpp
// 硬件：4 核 CPU
std::thread t1(func);
std::thread t2(func);
t1.join(); t2.join();
```

<details>
<summary>答案与复习指引</summary>

**两者都是**。创建了 2 个独立任务（并发设计），在 4 核 CPU 上可以真正同时执行（并行执行）。

复习：并发是"如何组织任务"，并行是"如何执行任务"。多线程在多核上同时满足两者。
</details>

### Q2: 下列代码有什么问题？

```cpp
int shared = 0;
void worker() {
    for (int i = 0; i < 100000; i++)
        shared++;  // 无同步
}
// 启动 4 个 worker 线程
```

<details>
<summary>答案与复习指引</summary>

**数据竞争（UB）**。`shared++` 不是原子操作（读-改-写三步），多线程同时执行会导致丢失更新。

修复：用 `std::atomic<int> shared{0};` 或加锁。

复习：数据竞争是未定义行为，不只是"可能出错"——编译器可能把 `shared` 缓存到寄存器，导致永远看不到更新。
</details>

### Q3: 下列哪个是并发但不是并行？

```cpp
// A:
std::thread t1(func), t2(func);
t1.join(); t2.join();  // 单核 CPU

// B:
while (true) {
    poll(fds, nfds, timeout);
    handle_ready_fds(fds);
}
// C: 两者都是
```

<details>
<summary>答案与复习指引</summary>

**A 是并发但不是并行**（单核上多线程只是时间片切换）。B 也是并发但不是并行（单线程事件循环）。A 和 B 都是"并发但不并行"的例子。

复习：并发不需要多核（时间片切换/事件循环都算），并行一定需要多核。
</details>

### Q4: 为什么 HFT 更关注"确定性"而非"绝对速度"？

<details>
<summary>答案与复习指引</summary>

HFT 的盈亏取决于在微秒级窗口内抢单。如果延迟偶尔从 5μs 飙到 50μs（数据竞争/锁竞争/缓存抖动），会导致错过交易窗口。**稳定的 10μs 比偶尔 5μs 但偶尔 50μs 更好**——可预测的延迟才能优化策略。

复习：并发引入数据竞争和锁竞争，这些是延迟抖动的主要来源。HFT 用绑核 + 无锁数据结构 + 固定线程池来保证确定性。
</details>

---

## 参考与延伸

- 下一节：[1.2 并发动机](02-motivation.md)
- 回到：[第 1 章 你好，并发](README.md)
