# 1.2 并发动机

> 第 1 章 · 上一节：[1.1 并发 vs 并行](01-concurrency-vs-parallelism.md) · 下一节：[1.3 并发风险](03-risks.md)

## 这节讲什么

为什么要并发？两个核心动机：**性能**（多核加速）和**响应性**（不阻塞主线程）。但并发不是免费的——Amdahl 定律告诉我们加速有上限，复杂性是代价。

## 为什么要学这个（先建立直觉）

先看 C 程序员的单线程困境：

```c
// C：单线程处理多个客户端
while (1) {
    int client = accept(server_fd, NULL, NULL);  // 阻塞等待连接
    char buf[1024];
    read(client, buf, sizeof(buf));   // 阻塞等待数据
    process(buf);                      // 处理时无法 accept 新连接
    write(client, result, len);        // 响应时也无法 accept
    close(client);
}
// 问题：一个慢客户端阻塞所有其他客户端
```

并发解决两个问题：

```cpp
// C++ 方案 1：多线程（性能 + 响应性）
void handle_client(int client) {
    char buf[1024];
    read(client, buf, sizeof(buf));
    process(buf);
    write(client, result, len);
    close(client);
}
// 每个客户端一个线程，互不阻塞
while (1) {
    int client = accept(server_fd, NULL, NULL);
    std::thread(handle_client, client).detach();
}

// C++ 方案 2：异步（响应性）
// 用 std::async 或事件循环，单线程管理多个 IO
```

## 两大动机详解

### 动机 1：性能（多核加速）

```cpp
// 串行：1 核跑 4 秒
for (int i = 0; i < 1000000; i++)
    data[i] = compute(i);  // 4 秒

// 并行：4 核各跑 1 秒
std::vector<std::thread> threads;
int chunk = 250000;
for (int t = 0; t < 4; t++) {
    threads.emplace_back([&, t] {
        for (int i = t * chunk; i < (t+1) * chunk; i++)
            data[i] = compute(i);
    });
}
for (auto& th : threads) th.join();
// 理论 1 秒，实际受 Amdahl 定律限制
```

**Amdahl 定律**：并行加速上限由串行部分决定。

```
加速比 = 1 / (S + (1-S)/N)

S = 串行比例（0~1）
N = 处理器数

例：90% 可并行，10% 串行，N=∞
加速比 = 1 / (0.1 + 0) = 10 倍（上限）
```

### 动机 2：响应性（不阻塞）

```cpp
// UI 线程不能阻塞——用户会感觉卡顿
void on_button_click() {
    // 错误：在 UI 线程做耗时操作
    // result = long_computation();  // UI 卡住 5 秒

    // 正确：后台线程做，UI 线程继续响应
    std::thread([] {
        auto result = long_computation();
        update_ui(result);  // 需要线程安全地更新 UI
    }).detach();
}
```

| 动机 | 目标 | 典型场景 | C 对应 |
|------|------|----------|--------|
| 性能 | 多核加速 | 大规模计算、数据处理 | pthread + 分治 |
| 响应性 | 不阻塞 | UI、网络 IO、实时系统 | select/poll/epoll |

## 常见错误（新手踩坑）

### 错误 1：忽视 Amdahl 定律

```cpp
// 错误：以为加线程就能无限加速
// 95% 并行 + 5% 串行
// 100 核理论加速 = 1/(0.05 + 0.95/100) ≈ 16.9 倍
// 不是 100 倍！串行 5% 就吃掉了大部分收益
```

**修复**：先分析串行瓶颈，减少串行比例再增加线程数。

### 错误 2：为了并发而并发

```cpp
// 错误：简单任务也开线程——线程创建开销 > 计算开销
void add(int a, int b) {
    std::thread([](int a, int b) { return a + b; }, a, b).join();
    // 创建线程 ~10-50μs，加法 ~1ns
}
```

**修复**：只在有明确收益时用并发。任务耗时 >> 线程创建开销才值得。

### 错误 3：IO 密集型用太多线程

```cpp
// 错误：1000 个客户端开 1000 个线程
// 每个线程栈 8MB → 8GB 内存！
for (int i = 0; i < 1000; i++)
    std::thread(handle_client, clients[i]).detach();
```

**修复**：IO 密集型用线程池（固定线程数）或异步 IO（epoll/io_uring），而非每连接一线程。

## 和 C 的区别

| 特性 | C | C++ |
|------|---|-----|
| 多线程 | pthread_create | std::thread |
| 异步 IO | select/poll/epoll | std::async / Asio 库 |
| 任务分解 | 手动分治 + pthread | std::thread + lambda |
| 线程安全 | 手动锁 + 原子操作 | std::mutex/atomic/condition_variable |
| 并发设计模式 | 手动实现 | 语言/标准库支持 |

## HFT 关联

- **行情接收 + 策略计算 + 风控 + 下单分到不同线程/核心**：消除串行瓶颈。行情线程专职收数据（不阻塞），策略线程专职计算（不等待 IO）。
- **Amdahl 定律在 HFT 的体现**：如果下单序列化（串行），无论行情/策略多快，下单吞吐就是瓶颈。要先优化串行部分。
- **响应性 = 不丢行情**：行情线程不能被策略计算阻塞——一秒钟收不到行情就可能错过交易机会。

## 代码自测

### Q1: 下列程序的理论加速比是多少？

```cpp
// 串行部分占 20%（数据加载 + 结果汇总）
// 并行部分占 80%（计算）
// 在 8 核 CPU 上运行
```

<details>
<summary>答案与复习指引</summary>

**Amdahl 定律**：加速比 = 1 / (S + (1-S)/N) = 1 / (0.2 + 0.8/8) = 1 / 0.3 ≈ 3.33 倍

8 核只获得 3.33 倍加速，因为 20% 串行部分吃掉了大量收益。即使无限核，上限也只有 1/0.2 = 5 倍。

复习：Amdahl 定律——串行部分决定加速上限。
</details>

### Q2: 下列代码有什么问题？

```cpp
void process_file(const std::string& path) {
    std::thread([path] {
        std::ifstream f(path);
        // ... 处理文件 ...
    }).detach();
}
// 调用方：process_file("data.txt"); 然后立即 return
```

<details>
<summary>答案与复习指引</summary>

**detach 后无法控制线程生命周期**。如果主线程退出，detached 线程被强制终止，可能数据丢失或资源泄漏。

修复：用 `join` 等待完成，或用线程池管理生命周期，或用 `std::future` 获取结果。

复习：detach 是"fire and forget"——只有在线程不需要返回结果且生命周期与进程一致时才安全。
</details>

### Q3: 下列哪种情况适合用多线程？

```cpp
// A: 计算 1+1
// B: 处理 100 万个元素的数组
// C: 读取一个文件并显示
// D: 同时监听 100 个网络连接
```

<details>
<summary>答案与复习指引</summary>

- **A 不适合**：计算太简单，线程创建开销远大于计算。
- **B 适合**：CPU 密集型，可分块并行。
- **C 不太适合**：IO 密集型，单线程 + 异步 IO 更好（除非文件特别大需并行处理）。
- **D 适合**：但用线程池（固定线程数）比每连接一线程更好。

复习：并发收益 = 任务耗时 - 线程创建/同步开销。只有正收益才值得并发。
</details>

### Q4: 为什么 HFT 不用 `std::async`？

<details>
<summary>答案与复习指引</summary>

`std::async` 可能内部创建临时线程（`launch::async` 策略），每次调用有线程创建开销和调度不确定性。HFT 需要固定的线程池 + 绑核 + 低延迟，`std::async` 无法保证。

HFT 通常自建线程池：启动时创建固定线程 + `pthread_setaffinity` 绑核 + 无锁队列通信。

复习：`std::async` 适合"方便但不需要极致性能"的场景。HFT 需要完全控制线程生命周期和调度。
</details>

---

## 参考与延伸

- 下一节：[1.3 并发风险](03-risks.md)
- 回到：[第 1 章](README.md)
