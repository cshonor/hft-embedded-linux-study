# 4.3 std::async 启动策略

> 第 4 章 · 上一节：[4.2 future/promise](02-future-promise.md) · 下一节：[4.4 future 的局限](04-future-limits.md)

## 这节讲什么

`std::async` 默认策略可能延迟到 `get()` 才同步执行——这是最容易被忽略的并发陷阱。要真正异步必须显式 `launch::async`。

## 为什么要学这个（先建立直觉）

C 程序员习惯了"创建线程就是立即执行"：

```c
// C：pthread_create 立即创建并运行线程
pthread_t t;
pthread_create(&t, NULL, work, NULL);  // 立即执行
// ... 做其他事 ...
pthread_join(t, NULL);  // 等待完成
```

但 `std::async` 的默认行为不是这样的——它**可能延迟到 `get()` 才执行**：

```cpp
// C++：std::async 默认策略的陷阱
auto f = std::async([] {
    std::cout << "running";
    return 42;
});
// 此时任务可能还没开始！
std::cout << "waiting";
int v = f.get();  // 可能在这里才同步执行！
// 输出可能是 "waitingrunning" 而不是 "runningwaiting"
```

## 三种策略详解

```cpp
// 1. launch::async——立即创建新线程执行
auto f1 = std::async(std::launch::async, [] {
    return compute();  // 立即在新线程中执行
});

// 2. launch::deferred——延迟到 get() 才执行（同步）
auto f2 = std::async(std::launch::deferred, [] {
    return compute();  // 不执行，等到 f2.get() 才在当前线程同步执行
});

// 3. launch::async | launch::deferred（默认）——实现自行选择
auto f3 = std::async([] {
    return compute();  // 可能 async 也可能 deferred——不确定！
});
```

### 默认策略的陷阱

```cpp
// 陷阱：默认策略可能是 deferred
auto f = std::async(work);  // 可能延迟到 get()

// 如果实现选择 deferred：
// 1. 任务不立即执行——不并发！
// 2. get() 时在当前线程同步执行——阻塞当前线程！
// 3. 如果当前线程是 UI 线程 → UI 卡住

// 更隐蔽的陷阱：析构阻塞
{
    auto f = std::async(work);  // 如果是 async 策略
    // 不调 get()
}  // f 析构 → 隐式 join → 阻塞等 work 完成
```

### 为什么默认策略是 async | deferred？

标准委员会的意图：让实现根据系统负载自行决定。如果系统繁忙，用 deferred（避免创建过多线程）；如果系统空闲，用 async。

但实际效果是：**不确定性**——同一个程序在不同机器/不同运行可能表现不同。这对调试和性能分析是灾难。

## 常见错误（新手踩坑）

### 错误 1：用默认策略期望异步

```cpp
// 错误：期望异步但可能同步
auto f = std::async(long_task);
do_other_work();  // 以为 long_task 在后台跑
int v = f.get();  // 可能 long_task 还没开始——在这里同步执行
```

**修复**：永远显式写 `std::launch::async`。

### 错误 2：循环中创建 async

```cpp
// 错误：循环中创建大量 async——可能创建过多线程
std::vector<std::future<int>> futures;
for (int i = 0; i < 10000; i++)
    futures.push_back(std::async(std::launch::async, work, i));
// 可能同时创建 10000 个线程 → 资源耗尽

// 修复：用线程池 + packaged_task
```

### 错误 3：forget to get（忘记取值）

```cpp
// 错误：不调 get()，future 析构时阻塞
{
    auto f = std::async(std::launch::async, [] {
        std::this_thread::sleep_for(std::chrono::seconds(10));
    });
    // 不调 get()
}  // f 析构 → 阻塞等 10 秒！
```

**修复**：如果不关心结果，用 `std::thread + detach`。如果关心，显式 `get()`。

## 和 C 的区别

| 特性 | C (pthread) | C++ (std::async) |
|------|-------------|-------------------|
| 启动 | 立即执行 | 可能延迟（默认策略） |
| 策略 | 无 | async/deferred/默认 |
| 返回值 | 手动管理 | future 自动 |
| 异常 | 不传播 | 自动传播 |
| 析构行为 | 不阻塞 | 阻塞（async 策略） |
| 线程管理 | 手动 | 实现自行决定 |

## HFT 关联

- **热路径意外阻塞**：HFT 异步任务若用默认 `async` 策略，可能被延迟到 `get()` 同步执行，热路径意外阻塞。务必 `launch::async`。
- **不用 async 创建线程**：`std::async` 可能内部使用线程池或创建新线程——不确定。HFT 需要完全控制线程创建和绑核，用 `std::thread` + 手动绑核。
- **析构阻塞是热路径陷阱**：HFT 代码中如果有 `async` 返回的 future 在热路径上析构，会隐式 join——引入不可预测的延迟。

## 代码自测

### Q1: 下列代码可能输出什么？

```cpp
auto f = std::async([] {
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    std::cout << "A";
    return 1;
});
std::cout << "B";
f.get();
std::cout << "C";
```

<details>
<summary>答案与复习指引</summary>

**不确定**。取决于默认策略：
- 如果 `launch::async`：可能输出 "BA C"（B 先打印，A 在后台，get 等待 A 完成后打印 C）或 "AB C"（A 先完成）
- 如果 `launch::deferred`：输出 "BAC"（B 先打印，get 触发同步执行 A，然后 C）

这就是默认策略的问题——行为不确定。

修复：显式写 `std::launch::async` 保证异步。

复习：默认策略 = async | deferred，实现自行选择。行为不可预测。
</details>

### Q2: 下列代码会阻塞多久？

```cpp
{
    auto f = std::async(std::launch::async, [] {
        std::this_thread::sleep_for(std::chrono::seconds(5));
    });
    // 不调 get()
}  // 这里会阻塞多久？
```

<details>
<summary>答案与复习指引</summary>

**阻塞 5 秒**。`launch::async` 返回的 future 在析构时会隐式 join——等待线程完成。即使不调 `get()`，析构也会阻塞。

这是 `std::async` 最反直觉的行为之一。

修复：如果不需要等待，用 `std::thread + detach`。如果需要结果，显式 `get()`。

复习：`async(launch::async)` 的 future 析构 = 隐式 join。
</details>

### Q3: 下列代码有什么问题？

```cpp
std::vector<std::future<int>> futures;
for (int i = 0; i < 1000; i++) {
    futures.push_back(std::async(std::launch::async, [i] {
        return heavy_compute(i);
    }));
}
// 逐个 get
for (auto& f : futures)
    result += f.get();
```

<details>
<summary>答案与复习指引</summary>

**可能创建 1000 个线程**。`launch::async` 每次创建新线程（或从实现内部线程池取），1000 个线程同时运行可能导致：
1. 内存耗尽（每线程栈 1-8MB）
2. 上下文切换开销
3. 缓存抖动

修复：用线程池限制并发数，或分批处理：
```cpp
// 分批：每批 4 个
for (int batch = 0; batch < 1000; batch += 4) {
    std::vector<std::future<int>> batch_futures;
    for (int i = batch; i < batch + 4 && i < 1000; i++)
        batch_futures.push_back(std::async(std::launch::async, heavy_compute, i));
    for (auto& f : batch_futures) result += f.get();
}
```

复习：`std::async` 不管理线程池——每次调用可能创建新线程。大量并发任务用线程池。
</details>

### Q4: 为什么 HFT 完全不用 std::async？

<details>
<summary>答案与复习指引</summary>

三个原因：
1. **线程控制**：`async` 可能用线程池或创建新线程——HFT 需要固定线程 + 绑核，不能接受不确定的线程管理。
2. **析构阻塞**：`async` 返回的 future 析构会 join——HFT 热路径上任何隐式阻塞都是灾难。
3. **堆分配**：promise/future 共享状态可能触发堆分配——HFT 禁止热路径堆分配。

HFT 替代方案：自建线程池 + 无锁队列 + 预分配结果缓冲区。

复习：`std::async` 适合"方便但不需要极致性能"的场景。HFT 需要完全控制线程和内存。
</details>

---

## 参考与延伸

- 下一节：[4.4 future 的局限](04-future-limits.md)
- 回到：[第 4 章](README.md)
