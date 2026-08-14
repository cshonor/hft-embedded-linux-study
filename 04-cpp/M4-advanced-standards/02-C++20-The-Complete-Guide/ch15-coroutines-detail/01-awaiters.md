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
