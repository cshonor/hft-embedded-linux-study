# 第 15 章 深入理解协程

**Coroutines in Detail**

## 本章讲什么

协程的底层机制：`promise_type` 的接口、`awaitable` 协议、协程帧生命周期、`symmetric transfer` 优化、自定义 awaitable。

## 要点

### `promise_type` 的接口

协程返回类型必须包含一个 `promise_type`，编译器通过它控制协程行为：

```cpp
struct MyTask {
    struct promise_type {
        // 协程开始时调用
        MyTask get_return_object();
        // 初始是否挂起
        std::suspend_always initial_suspend();
        // 结束时是否挂起（通常挂起以延迟释放）
        std::suspend_always final_suspend() noexcept;
        // co_yield 的值处理
        std::suspend_always yield_value(T v);
        // co_return 的值处理
        void return_value(T v);
        void return_void();
        // 异常处理
        void unhandled_exception();
    };
};
```

### `awaitable` 协议

`co_await expr` 要求 `expr` 是 **awaitable**——实现三个函数：

```cpp
struct MyAwaitable {
    bool await_ready() { return false; }  // 是否已完成（true 则不挂起）
    void await_suspend(std::coroutine_handle<> h) {
        // 挂起时调用：通常启动异步操作，操作完成后 h.resume()
        start_async(h);
    }
    T await_resume() { return result_; }  // 恢复时返回的值（co_await 的结果）
};

// 使用
T val = co_await MyAwaitable{};
```

### 协程帧与 `coroutine_handle`

```cpp
// coroutine_handle：协程帧的句柄
std::coroutine_handle<MyTask::promise_type> handle;

handle.resume();      // 恢复协程
handle.destroy();     // 销毁协程帧
handle.done();        // 是否已结束
handle.promise();     // 访问 promise 对象
```

`coroutine_handle` 是协程帧的指针——手动管理生命周期，像裸指针，要小心 use-after-free。

### 协程帧的生命周期

```
协程开始 → 分配协程帧（堆） → 初始化 promise → initial_suspend
    → 执行到 co_await/co_yield → 保存状态 → await_suspend → 返回调用方
    → [外部 resume()] → 恢复状态 → 从暂停点继续
    → co_return → final_suspend → 释放协程帧
```

**关键**：协程帧的生命周期跨挂起/恢复——局部变量保存在帧上，不是栈。帧不释放，变量一直在。

### Symmetric Transfer（对称转移）

```cpp
// 问题：协程 A co_await 协程 B，B 结束后恢复 A → 栈增长（递归 resume）
// 优化：B 结束时不 resume A，而是"转移"给 A（不增加栈深度）

std::coroutine_handle<> final_suspend() noexcept {
    return caller_handle;   // 对称转移，而非 caller.resume()
}
```

对称转移避免递归 resume 导致栈溢出——生成器/链式协程必备优化。

### 协程帧的分配优化

```cpp
struct promise_type {
    // 重载 operator new 返回静态分配的帧
    void* operator new(std::size_t) { return pool.alloc(); }
    void operator delete(void* p) { pool.free(p); }
};
```

- 默认堆分配 `new`。
- 可重载 `operator new` 用 mempool/arena，热路径可控。
- **RAII 优化**：如果编译器能证明协程帧不逃逸（`suspend_always` + 局部使用），可能省去堆分配（HALO，Heap Allocation eLision Optimization）。

## HFT 关联

- **自定义 awaitable 封装 SPSC 队列**：`co_await queue.pop_async()` 内部 `await_suspend` 注册回调，数据来了 `resume`——异步消息处理。
- **协程帧池化**：`promise_type::operator new` 从 mempool 分配协程帧，避免堆 malloc。
- **symmetric transfer 防栈溢出**：生成器链式 `co_yield` 用对称转移，避免深层递归。
- **`coroutine_handle` 生命周期警惕**：handle 是裸指针，协程帧释放后 resume 是 UB——用 RAII 包装。
- **管理通道用协程，热路径不用**：协程帧分配 + 状态机有开销，纳秒热路径仍同步。管理通道（配置拉取、监控）异步 IO 用协程代码清晰。
- **`asio` 协程集成**：`asio::awaitable` + `co_await` 是 C++20 网络编程的事实标准，HFT 管理通道可用。

## 自测题

1. `promise_type` 必须实现哪些函数？各自的作用？
2. `awaitable` 协议的三个函数是什么？`await_resume` 返回什么？
3. `coroutine_handle` 的作用？为什么说它像裸指针？
4. symmetric transfer 解决什么问题？不用会怎样？
5. HFT 如何让协程帧从 mempool 分配？为什么热路径仍不用协程？
