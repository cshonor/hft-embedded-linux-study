# 2.4 RAII 守卫

> 第 2 章 · 上一节：[2.3 转移所有权](03-transferring-ownership.md) · 下一节：[2.5 硬件并发数](05-hardware-concurrency.md)

## 这节讲什么

用 RAII 包装 `std::thread`，保证所有路径（含异常）都安全收尾。C++20 的 `std::jthread` 是标准化版本，带协作式中断。

## 为什么要学这个（先建立直觉）

C 程序员对"忘了 cleanup"很熟悉：

```c
// C：忘了 pthread_join，资源泄漏但不崩
void buggy() {
    pthread_t t;
    pthread_create(&t, NULL, func, NULL);
    if (error) return;  // 忘了 pthread_join → 泄漏
    pthread_join(t, NULL);
}
```

C++ 的 `std::thread` 更严格——忘了 join 就 `terminate`。但每次手动 join 很容易在异常路径遗漏。RAII 守卫解决了这个问题：

```cpp
// 没有 RAII：异常路径容易遗漏
void risky() {
    std::thread t(long_task);
    do_work();  // 抛异常 → t 析构 → terminate
    t.join();
}

// 有 RAII：所有路径自动 join
void safe() {
    std::thread t(long_task);
    joining_thread guard(std::move(t));  // RAII 守卫接管
    do_work();  // 即使抛异常，guard 析构也会 join
    // guard 析构时自动 join
}
```

## RAII 守卫实现详解

### 自制 joining_thread

```cpp
class joining_thread {
    std::thread t;
public:
    joining_thread() noexcept = default;  // 空构造

    template<typename F, typename... Args>
    explicit joining_thread(F&& f, Args&&... args)
        : t(std::forward<F>(f), std::forward<Args>(args)...) {}

    explicit joining_thread(std::thread&& th) noexcept
        : t(std::move(th)) {}

    joining_thread(joining_thread&&) noexcept = default;  // 可移动
    joining_thread& operator=(joining_thread&&) noexcept = default;

    // 禁止拷贝
    joining_thread(const joining_thread&) = delete;
    joining_thread& operator=(const joining_thread&) = delete;

    ~joining_thread() {
        if (t.joinable()) t.join();  // 析构时自动 join
    }

    // 也可以 detach 或手动 join
    void join() { t.join(); }
    void detach() { t.detach(); }
    bool joinable() const noexcept { return t.joinable(); }
};
```

### 使用方式

```cpp
// 方式 1：直接构造
joining_thread t([] { long_task(); });

// 方式 2：从 std::thread 移动
std::thread raw(func);
joining_thread t2(std::move(raw));

// 方式 3：赋值
joining_thread t3;
t3 = joining_thread([] { another_task(); });

// 所有方式：析构时自动 join，异常路径也安全
```

### C++20 std::jthread

```cpp
// C++20：标准化的 joining_thread + 协作式中断
#include <stop_token>

std::jthread t([](std::stop_token st) {
    while (!st.stop_requested()) {
        do_work();
    }
});

// 请求停止
t.request_stop();
// jthread 析构时：request_stop() + join()
```

`std::jthread` 相比自制守卫多了：
- 析构时先 `request_stop()` 再 `join()`（不只是盲目 join）
- `stop_token` 协作式中断机制
- 可检查 `stop_requested()` 做优雅退出

## 常见错误（新手踩坑）

### 错误 1：守卫拷贝导致双重 join

```cpp
// 如果不 delete 拷贝构造：
class bad_guard {
    std::thread t;
public:
    ~bad_guard() { if (t.joinable()) t.join(); }
    // 没有 delete 拷贝构造！
};

bad_guard g1(std::thread(func));
bad_guard g2 = g1;  // 拷贝！
// g1 和 g2 析构时都尝试 join 同一线程 → 第二个 join 是 UB
```

**修复**：`delete` 拷贝构造和拷贝赋值。

### 错误 2：守卫析构时 join 导致死锁

```cpp
class Worker {
    std::jthread t;
    std::mutex mtx;
public:
    Worker() : t([this] { run(); }) {}

    void run() {
        while (true) {
            std::lock_guard<std::mutex> lk(mtx);  // 等 mtx
            // ...
        }
    }

    ~Worker() {
        // jthread 析构时 request_stop() + join()
        // 但如果析构时持有 mtx → 线程等 mtx → join 等线程 → 死锁！
    }
};
```

**修复**：析构前手动释放锁，或在线程函数中检查 stop_token。

### 错误 3：jthread 的 stop_token 没检查

```cpp
// 错误：线程不检查 stop_requested，request_stop 无效
std::jthread t([](std::stop_token st) {
    while (true) {  // 没检查 st.stop_requested()
        blocking_read();  // 阻塞在 IO 上，request_stop 无法唤醒
    }
});
```

**修复**：在循环条件检查 `stop_requested()`，或用 `stop_callback` 注册回调来打断阻塞。

## 和 C 的区别

| 特性 | C (pthread) | C++ (RAII 守卫) | C++20 (jthread) |
|------|-------------|------------------|------------------|
| 自动 join | 无（手动） | 析构时 join | 析构时 join |
| 异常安全 | 无 | 有 | 有 |
| 协作式中断 | 无 | 无 | stop_token |
| 析构行为 | — | 盲目 join | request_stop + join |
| 拷贝保护 | 无 | delete | delete |

## HFT 关联

- **守护进程防崩**：HFT 守护进程里 `std::thread` 析构 terminate 会拉崩进程，用 RAII 守卫或 C++20 `jthread` 保证安全。
- **优雅退出**：HFT 关闭时需要通知所有线程停止（保存状态、刷新订单）→ `jthread` 的 `stop_token` 是理想机制。
- **jthread 的局限**：`request_stop()` 只是设置标志，不抢占。如果线程在 `mutex::lock()` 上阻塞，`stop_token` 无法唤醒它——HFT 需要超时锁 + 定期检查。

## 代码自测

### Q1: 下列代码安全吗？

```cpp
class Task {
    std::thread t;
public:
    Task() : t([this] { this->run(); }) {}
    ~Task() { if (t.joinable()) t.join(); }
    void run() { /* ... */ }
};

Task task;
// Task 析构时会发生什么？
```

<details>
<summary>答案与复习指引</summary>

**可能有死锁**。如果 `run()` 访问 `Task` 的成员，而 `~Task()` 在 `run()` 使用成员时开始析构（先析构成员再 join），顺序错误。

修复：确保 join 在成员析构前完成。`~Task()` 中 `t.join()` 在函数体（成员析构前），所以如果 `run()` 只用 Task 的成员（不在析构期间），是安全的。但如果 `run()` 调用虚函数或其他依赖对象完整性的操作，仍有风险。

复习：RAII 守卫的析构顺序——先 join 线程，再析构成员。确保线程不依赖正在析构的对象。
</details>

### Q2: 下列代码有什么问题？

```cpp
void work() {
    joining_thread t1([] { task1(); });
    joining_thread t2([] { task2(); });
    // t2 先析构（LIFO），等 task2 完成
    // t1 后析构，等 task1 完成
}
```

<details>
<summary>答案与复习指引</summary>

**没有功能问题，但有性能问题**。t2 析构时 join task2，期间 task1 可能已完成但无法被 join（要等 t2 先析构）。串行 join 浪费时间。

如果 task1 和 task2 可以并行结束，总时间是 task2 + task1 而非 max(task1, task2)。

修复（如需优化）：先 detach 或用 future，或改变析构顺序。但通常 LIFO join 是可接受的。

复习：RAII 守卫按 LIFO 顺序析构——后构造的先 join。
</details>

### Q3: 下列 jthread 代码能优雅退出吗？

```cpp
std::jthread t([](std::stop_token st) {
    while (!st.stop_requested()) {
        process_queue();  // 可能阻塞 1 秒
    }
});
// 主线程：t = std::jthread{};  // 触发 request_stop + join
```

<details>
<summary>答案与复习指引</summary>

**能退出，但有延迟**。`request_stop()` 设置标志，但 `process_queue()` 如果正在阻塞，要等它返回后才能检查 `stop_requested()`。最坏情况延迟 1 秒。

如果需要立即退出，`process_queue` 内部需要检查 `stop_token` 或用超时 + `stop_callback`。

复习：`stop_token` 是协作式中断——只设置标志，不抢占。线程必须主动检查。
</details>

### Q4: 为什么 HFT 不完全依赖 jthread 的自动 join？

<details>
<summary>答案与复习指引</summary>

1. **盲目 join 可能阻塞**：HFT 关闭时需要超时——如果某线程卡死，join 会永远阻塞。HFT 需要超时检测 + 强制退出机制。
2. **stop_token 不抢占**：如果线程在 `mutex::lock()` 上阻塞，`request_stop` 无法唤醒它。
3. **确定性退出**：HFT 需要按固定顺序停止线程（先停策略→再停行情→最后停 IO），jthread 的自动析构是 LIFO 顺序，不一定匹配。

HFT 通常自建退出流程：发停止信号→等超时→记录未完成的线程→强制退出。

复习：jthread 适合"正常退出"场景。HFT 需要更健壮的异常退出机制。
</details>

---

## 参考与延伸

- 下一节：[2.5 硬件并发数](05-hardware-concurrency.md)
- 回到：[第 2 章 管理线程](README.md)
