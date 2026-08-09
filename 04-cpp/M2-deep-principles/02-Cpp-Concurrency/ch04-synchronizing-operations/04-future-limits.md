# 4.4 future 的局限

> 第 4 章 · 上一节：[4.3 async 启动策略](03-async-policy.md) · 下一节：[4.5 超时等待](05-timeout.md)

## 这节讲什么

future 有三个局限：只能取一次、不能轮询、析构阻塞（async 返回的 future）。理解局限才能正确选择同步机制。

## 为什么要学这个（先建立直觉）

C 程序员习惯了"检查状态再取值"的轮询模式：

```c
// C：可以轮询检查结果是否就绪
ResultChannel ch;
start_worker(&ch);

while (!ch.done) {
    do_other_work();  // 不阻塞，做其他事
}
printf("%d\n", ch.result);
```

C++ 的 `future` 不支持这种模式——没有 `try_get`，要么阻塞 `get()`，要么超时 `wait_for`：

```cpp
// C++：future 不能轮询
std::future<int> f = std::async(std::launch::async, work);

// 不能这样做：没有 try_get
// if (f.is_ready()) { auto v = f.try_get(); }

// 只能阻塞：
int v = f.get();  // 阻塞直到完成

// 或超时等待：
if (f.wait_for(std::chrono::milliseconds(100)) == std::future_status::ready)
    int v = f.get();  // 此时 get 不阻塞
else
    // 超时，还没完成
```

## 三大局限详解

### 局限 1：只能取一次

```cpp
std::future<int> f = std::async(std::launch::async, [] { return 42; });

int v1 = f.get();  // OK，返回 42
int v2 = f.get();  // UB！future 已移空

// 多读者用 shared_future
auto sf = f.share();  // 但 f 已经 get 过了，不能 share
// 必须在 get 之前 share
```

### 局限 2：不能轮询

```cpp
// 标准库没有 is_ready()——但可以用 wait_for + 零超时模拟
bool is_ready(std::future<int>& f) {
    return f.wait_for(std::chrono::seconds(0)) == std::future_status::ready;
}

auto f = std::async(std::launch::async, work);
while (!is_ready(f)) {
    do_other_work();  // 轮询期间做其他事
}
int v = f.get();  // 不阻塞，已完成
```

**注意**：这种轮询方式效率低——每次 `wait_for(0)` 都有系统调用开销。

### 局限 3：析构阻塞

```cpp
{
    auto f = std::async(std::launch::async, [] {
        std::this_thread::sleep_for(std::chrono::seconds(10));
    });
    // 不调 get()
}  // ← f 析构，隐式 join，阻塞 10 秒！

// 原因：async 返回的 future 析构时会等待线程完成
// 这是标准的有意设计——防止 detached 线程访问已销毁的局部变量
```

### 为什么有这些局限？

`future` 的设计哲学是**一次性结果传递**——简单、安全、不可重复。

```
future 的生命周期：
  创建 ──→ 等待 ──→ get（一次性）──→ 失效
                          ↑
                     只能在这里取值

对比 shared_future：
  创建 ──→ share ──→ 多次 get ──→ 持续有效
```

| 局限 | 原因 | 替代方案 |
|------|------|----------|
| 只能取一次 | 一次性语义 | `shared_future` |
| 不能轮询 | 设计简化 | `wait_for(0)` 模拟 |
| 析构阻塞 | 防止悬垂访问 | `shared_future` 或 `thread+detach` |

## 常见错误（新手踩坑）

### 错误 1：以为不调 get 就不阻塞

```cpp
// 错误：以为不调 get() 就不等待
void buggy() {
    auto f = std::async(std::launch::async, long_task);
    // 不调 get()
}  // ← 析构阻塞！等 long_task 完成

// 修复方案 1：调 get()
void fix1() {
    auto f = std::async(std::launch::async, long_task);
    // ... 做其他事 ...
    f.get();  // 显式等待
}

// 修复方案 2：用 thread + detach
void fix2() {
    std::thread(long_task).detach();  // 真正的后台执行
}
```

### 错误 2：vector<future> 析构串行阻塞

```cpp
// 错误：vector 析构时逐个 join——串行等待
{
    std::vector<std::future<void>> futures;
    for (int i = 0; i < 4; i++)
        futures.push_back(std::async(std::launch::async, task_i));
}  // 逐个析构：等 task0 → 等 task1 → 等 task2 → 等 task3
// 总时间 = task0 + task1 + task2 + task3（串行！）

// 修复：显式循环 get（仍然是串行，但至少你能控制顺序）
for (auto& f : futures) f.get();
// 或用 barrier/latch 等待所有完成再析构
```

### 错误 3：shared_future 在 get 前就 share

```cpp
std::future<int> f = std::async(std::launch::async, [] { return 42; });
int v = f.get();  // 先 get
auto sf = f.share();  // 错误！f 已移空
// sf.get() 抛异常
```

**修复**：先 share 再 get：
```cpp
auto sf = f.share();  // 先 share
int v1 = sf.get();    // 42
int v2 = sf.get();    // 42（shared_future 可多次 get）
```

## 和 C 的区别

| 特性 | C | C++ |
|------|---|-----|
| 轮询 | 手动检查 flag | `wait_for(0)` 模拟 |
| 多次取值 | 手动保留结果 | `shared_future` |
| 析构行为 | 无（手动管理） | 阻塞（async future） |
| 超时 | 手动 timer | `wait_for`/`wait_until` |

## HFT 关联

- **future 析构阻塞陷阱**：`async` 返回的 future 析构会 join，热路径上误用会导致隐式串行化。
- **不能轮询的局限**：HFT 需要非阻塞检查结果（如检查行情是否更新），`future` 的 `wait_for(0)` 有系统调用开销——用 `atomic` 序列号替代。
- **vector<future> 的串行析构**：HFT 启动时并行初始化多个组件，如果用 `vector<future>` 存结果，析构时串行等待——用 `latch` 或 `barrier` 替代。

## 代码自测

### Q1: 下列代码会阻塞多久？

```cpp
auto f1 = std::async(std::launch::async, [] {
    std::this_thread::sleep_for(std::chrono::seconds(3));
});
auto f2 = std::async(std::launch::async, [] {
    std::this_thread::sleep_for(std::chrono::seconds(5));
});
// 不调 get()
// f1 和 f2 在此处析构
```

<details>
<summary>答案与复习指引</summary>

**阻塞约 5 秒**（不是 8 秒）。f1 和 f2 析构时，f1 先析构等待 3 秒，然后 f2 析构——但 f2 的任务在这 3 秒里一直在跑，所以 f2 析构时只需要再等 2 秒（5-3=2）。总阻塞约 5 秒。

但这是最乐观的情况——实际取决于调度。如果两个线程在同一个核上时间片切换，可能更长。

复习：future 析构是串行的（LIFO），但后台任务在并行执行——析构等待时间是 max(剩余任务时间)。
</details>

### Q2: 下列代码正确吗？

```cpp
auto f = std::async(std::launch::async, [] { return 42; });
auto v1 = f.get();
auto v2 = f.get();  // 第二次 get
```

<details>
<summary>答案与复习指引</summary>

**不正确**。`future::get()` 只能调一次——第一次 get 后 future 进入无效状态，第二次 get 是 UB（通常抛 `std::future_error(no_state)`）。

修复：用 `shared_future`：
```cpp
auto sf = std::async(std::launch::async, [] { return 42; }).share();
auto v1 = sf.get();  // 42
auto v2 = sf.get();  // 42
```

复习：future = 一次性，shared_future = 可多次。
</details>

### Q3: 如何实现非阻塞检查 future 是否完成？

<details>
<summary>答案与复习指引</summary>

用 `wait_for` + 零超时：
```cpp
bool is_ready(std::future<int>& f) {
    return f.wait_for(std::chrono::seconds(0)) == std::future_status::ready;
}
```

注意：这不是真正的轮询——每次调用有系统调用开销。如果需要高频检查，用 `atomic` 序列号替代。

HFT 更好的方案：
```cpp
std::atomic<bool> done{false};
std::thread([&] {
    compute();
    done.store(true, std::memory_order_release);
}).detach();

while (!done.load(std::memory_order_acquire))
    _mm_pause();  // 自旋检查
```

复习：`wait_for(0)` 是标准库提供的"伪轮询"——有开销。HFT 用 atomic 替代。
</details>

### Q4: 为什么 vector<future> 析构是串行的？如何优化？

<details>
<summary>答案与复习指引</summary>

vector 析构时按逆序逐个调用 `~future()`，每个析构等待对应线程完成——串行等待。

优化方案 1：先循环 wait_for 等所有完成，再析构：
```cpp
for (auto& f : futures)
    f.wait();  // 等所有完成（并行执行，串行等待但等待时间 = max）
// 此时析构不阻塞（都已完成）
```

优化方案 2：用 `latch` 等待所有完成：
```cpp
std::latch done(n);
for (int i = 0; i < n; i++)
    threads.emplace_back([&] { work(); done.count_down(); });
done.wait();  // 等所有完成
```

复习：vector<future> 析构串行等待。先 wait 再析构可以让等待时间 = max 而非 sum。
</details>

---

## 参考与延伸

- 下一节：[4.5 超时等待](05-timeout.md)
- 回到：[第 4 章](README.md)
