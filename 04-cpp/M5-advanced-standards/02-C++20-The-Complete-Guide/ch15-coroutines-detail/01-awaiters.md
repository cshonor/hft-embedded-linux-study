# Awaiter 与 awaitable

## co_await 的机制

```cpp
// co_await expr 的展开：
auto&& awaitable = expr;
auto&& awaiter = promise.await_transform(awaitable);
// 或直接 awaitable.get_awaitable() 如果没有 await_transform

if (!awaiter.await_ready()) {
    // 暂停协程
    awaiter.await_suspend(coroutine_handle);
    // 返回到调用者
}

// 恢复后：
auto result = awaiter.await_resume();
```

## Awaiter 接口

```cpp
struct MyAwaiter {
    bool await_ready() {
        // 是否可以立即完成（不用暂停）
        return false;  // 需要暂停
    }

    void await_suspend(std::coroutine_handle<> h) {
        // 暂停时执行
        // 通常：注册回调、启动异步操作
        // 异步操作完成后调用 h.resume() 恢复
    }

    T await_resume() {
        // 恢复后返回的值
        return result;
    }
};
```

## suspend_always / suspend_never

```cpp
// 标准库提供两个简单 awaiter
std::suspend_always{};  // 总是暂停
std::suspend_never{};   // 从不暂停

// 用在 promise_type 中
struct promise_type {
    auto initial_suspend() { return std::suspend_always{}; }  // 创建后立即暂停
    auto initial_suspend() { return std::suspend_never{}; }   // 创建后立即运行
    auto final_suspend() noexcept { return std::suspend_always{}; }  // 结束后暂停（等销毁）
};
```

## 自定义 awaiter：异步 IO

```cpp
struct AsyncReadAwaiter {
    int fd;
    char* buf;
    size_t len;
    ssize_t result;

    bool await_ready() { return false; }

    void await_suspend(std::coroutine_handle<> h) {
        // 注册异步读，完成后恢复协程
        async_read(fd, buf, len, [this, h](ssize_t n) {
            result = n;
            h.resume();
        });
    }

    ssize_t await_resume() { return result; }
};

// 使用
task<void> process() {
    char buf[1024];
    ssize_t n = co_await AsyncReadAwaiter{fd, buf, sizeof(buf)};
    // 异步读完成后继续
    process_data(buf, n);
}
```

## 协程与线程

```cpp
// 协程不是线程——协程在单个线程上暂停/恢复
// 协程是协作式的（手动让出），线程是抢占式的

// 协程的优势：
// 1. 零开销上下文切换（只是保存/恢复寄存器）
// 2. 无锁（单线程协程无数据竞争）
// 3. 高并发（一个线程可以跑大量协程）

// HFT：
// - 不用协程做热路径（恢复延迟不确定）
// - 适合异步 IO（等待网卡数据时不阻塞线程）
// - 适合状态机（用协程实现协议解析状态机）
```

## 自测题

1. `co_await` 的三个步骤是什么？（ready/suspend/resume）
2. `suspend_always` 和 `suspend_never` 的区别？
3. 自定义 awaiter 的三个方法分别做什么？
4. 协程和线程的区别？协程的优势是什么？
5. HFT 中协程适合什么场景？不适合什么场景？

<details>
<summary>参考答案</summary>

1. `co_await expr` 展开为三步（由 awaiter 的三个方法驱动）：
   1. **`await_ready()`**：先问"结果是否已经就绪"。若返回 `true`，说明不必挂起，**直接跳到第 3 步**（同步完成，零挂起开销）。
   2. **挂起**：若返回 `false`，把状态写入协程帧并挂起，然后调用 **`await_suspend(coroutine_handle<> h)`**——它负责把 `h` 交给异步操作，等操作完成后由某处调用 `h.resume()`。
   3. **恢复**：协程被 resume 后调用 **`await_resume()`**，其返回值就是整个 `co_await` 表达式的值（异常也从这里传出）。
2. 它们是标准库提供的两个最简 awaiter，区别只在 `await_ready()` 的返回值：
   - `std::suspend_always`：`await_ready()` 恒返回 **`false`** —— **总是挂起**（常用于 `initial_suspend` 让协程惰性启动，或 `final_suspend` 保留协程以便销毁/取结果）。
   - `std::suspend_never`：`await_ready()` 恒返回 **`true`** —— **从不挂起**，立即继续（让协程一调用就跑到第一个真正的挂起点）。
两者都不保存状态，纯粹是"挂起策略"的标记。
3. 自定义 awaiter 需要三个方法：
   - **`await_ready()`** → bool：结果是否已就绪；返回 true 则跳过挂起（快路径）。
   - **`await_suspend(std::coroutine_handle<> h)`**：挂起后调用，负责**安排后续的恢复**（如把 `h` 注册到事件循环/IO 完成回调里）。返回 `void` 表示把控制权交回调用者；返回 `bool` 时，`false` 表示立即恢复协程。
   - **`await_resume()`**：恢复后调用，其**返回值就是 `co_await` 表达式的值**；若异步操作失败，也在这里抛异常。
```cpp
struct AsyncReadAwaiter {
    bool await_ready() { return false; }
    void await_suspend(std::coroutine_handle<> h) { /* 注册回调，完成时 h.resume() */ }
    ssize_t await_resume() { return result; }
};
```
4. 协程**不是**线程：
   - 协程在**线程之上**运行，切换是**协作式**的（只在 `co_await`/`co_yield` 处让出），线程是**抢占式**的（OS 随时可打断）。
   - 协程切换只需保存/恢复少量状态并跳转，**不进内核**，没有线程切换的调度器/TLB/cache 污染开销；线程切换要陷入内核。
   - 一个线程上可以跑成千上万个协程；单线程内的协程之间**不存在数据竞争**，不需要锁。
   - 协程的挂起点是显式的（写在代码里），栈是"可挂起的帧"而不是固定的线程栈。
优势：切换开销极小、无锁并发、高并发密度、异步代码可以写成同步顺序风格（可读性好）。
5. **适合**：
   - **异步 I/O**：等网卡/磁盘/数据库时挂起，不占用线程，一个线程就能扛大量连接；
   - **状态机**：把协议解析/多阶段流程写成顺序代码，用 `co_await` 表达"等待下一步"，比手写状态枚举清晰得多；
   - **旁路/离线逻辑**：批量回放、配置加载、日志落盘等非关键路径。
**不适合**：
   - **热路径/撮合主循环**：协程帧的分配与恢复时机由调度器决定，**尾延迟不确定**；挂起/恢复还要访问帧（可能 cache miss），这与低延迟要求的确定性相冲突；
   - 需要严格绑核、精确控制内存布局的场景。
一句话：协程用来解决"**等**"的问题，不适合解决"**快**"的问题。

</details>
