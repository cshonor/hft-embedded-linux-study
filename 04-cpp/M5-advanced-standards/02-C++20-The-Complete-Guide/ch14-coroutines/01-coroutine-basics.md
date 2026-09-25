# 协程基础

## 什么是协程

```cpp
#include <coroutine>

// 协程：可以暂停和恢复的函数
// 关键字：co_await、co_yield、co_return

// 生成器：每次调用产出一个值
generator<int> count(int n) {
    for (int i = 0; i < n; ++i) {
        co_yield i;  // 产出 i，暂停
    }
    co_return;  // 结束
}

// 使用
for (int x : count(5)) {
    std::cout << x << ' ';  // 0 1 2 3 4
}
```

## co_await / co_yield / co_return

```cpp
// co_await：等待一个异步操作完成
task<int> fetch_data() {
    auto data = co_await async_read();  // 暂停，等异步操作完成
    co_return data;  // 返回结果
}

// co_yield：产出值，暂停
generator<int> gen() {
    co_yield 1;  // 产出 1，暂停
    co_yield 2;  // 产出 2，暂停
    co_yield 3;  // 产出 3，暂停
}

// co_return：结束协程
task<void> do_work() {
    // ...
    co_return;  // 结束
}
```

## 协程机制

```cpp
// 协程被编译器变换为：
// 1. 在堆上分配协程帧（保存局部变量、暂停点）
// 2. 返回一个 handle/promise 对象
// 3. 暂停时保存状态到帧，返回到调用者
// 4. 恢复时从帧恢复状态，继续执行

// 核心组件：
// - promise_type：控制协程行为
// - coroutine_handle：恢复/销毁协程的句柄
// - awaiter：co_await 的操作数，定义暂停/恢复逻辑
```

## 简化生成器

```cpp
// C++20 标准库没有内置 generator，需要自己实现或用库
// 简化版：
template <typename T>
struct generator {
    struct promise_type {
        T current_value;
        auto get_return_object() { return generator{handle_type::from_promise(*this)}; }
        auto initial_suspend() { return std::suspend_always{}; }
        auto final_suspend() noexcept { return std::suspend_always{}; }
        auto yield_value(T v) { current_value = v; return std::suspend_always{}; }
        void return_void() {}
        void unhandled_exception() { std::terminate(); }
    };
    using handle_type = std::coroutine_handle<promise_type>;
    handle_type handle;

    struct iterator {
        handle_type h;
        iterator& operator++() { h.resume(); return *this; }
        T& operator*() { return h.promise().current_value; }
        bool operator!=(std::default_sentinel_t) { return !h.done(); }
    };
    iterator begin() { handle.resume(); return {handle}; }
    std::default_sentinel_t end() { return {}; }
};

generator<int> fibonacci() {
    int a = 0, b = 1;
    while (true) {
        co_yield a;
        auto next = a + b;
        a = b;
        b = next;
    }
}
```

## 自测题

1. 协程和普通函数的区别？三个协程关键字是什么？
2. `co_yield` 和 `co_return` 的区别？
3. 协程的状态保存在哪里？暂停和恢复怎么实现？
4. `promise_type` 的作用是什么？
5. 如何用协程实现无限斐波那契序列？

<details>
<summary>参考答案</summary>

1. 普通函数一路执行到 `return` 才把控制权交回调用者，且返回后其局部变量全部消失。
协程可以在执行中途**挂起（suspend）**，把控制权交回调用者/恢复者，**之后再从挂起点继续**，且局部变量在挂起期间保持有效。
三个协程关键字：`co_await`（挂起并等待一个 awaitable，恢复后得到其结果）、`co_yield`（挂起并产出一个值，之后还能继续）、`co_return`（结束协程并返回结果）。函数体内出现任意一个，它就成了协程。
2. `co_yield expr`：**挂起并产出**一个值给调用者（编译器把它翻译成 `co_await promise.yield_value(expr)`），协程之后**还能被恢复继续执行**——这是 generator 的写法。
`co_return [expr]`：**结束**协程，把值交给 `promise.return_value(...)`（或 `return_void()`），随后进入 final suspend，协程生命周期结束，不再继续。
简言之：`co_yield` 是"暂时交出"，`co_return` 是"结束"。
3. 状态保存在**协程帧（coroutine frame）**里：局部变量、临时对象、挂起点位置（恢复时从哪一行继续）、promise 对象等都在这个帧中。帧通常在**堆上**分配（编译器在能证明生命周期时可以进行省略优化）。
   - **暂停**：把挂起点的位置与需要跨越挂起点的寄存器/局部状态写入帧，然后通过 awaiter 的 `await_suspend()` 安排"将来由谁来恢复"，随即返回调用者。
   - **恢复**：调用 `coroutine_handle::resume()`，从帧里恢复状态并跳回挂起点继续执行。
因为状态在帧里而不在调用栈上，协程可以在函数返回后仍然"活着"。
4. 它是协程的**控制器兼对外接口**，定义了协程的全部行为。编译器通过返回类型里的嵌套类型 `promise_type` 找到它，并按需调用：
   - `get_return_object()`：决定协程返回给调用者的那个对象（如 `generator<T>`）；
   - `initial_suspend()`：是否**一开始就挂起**（`suspend_always` → 惰性，调用后不执行）；
   - `final_suspend()`：结束后是否挂起（便于在销毁前取结果/转移控制权）；
   - `yield_value(v)`：处理 `co_yield`；
   - `return_value(v)` / `return_void()`：处理 `co_return`；
   - `unhandled_exception()`：协程内抛出未捕获异常时的处理；
   - （可选）`await_transform()`：定制 `co_await` 的行为。
换句话说：协程体写"流程"，`promise_type` 写"语义"。
5. 用 `co_yield` 把每个值"吐"给调用者，配合一个最小的 `generator`：
```cpp
generator<int> fibonacci() {
    int a = 0, b = 1;
    while (true) {
        co_yield a;              // 产出当前值并挂起
        auto next = a + b;
        a = b; b = next;
    }
}
for (int v : fibonacci() | std::views::take(10)) { /* 0,1,1,2,3,5,... */ }
```
要点：`promise_type::initial_suspend()` 返回 `suspend_always{}`（调用时不执行，首次迭代才开始），`yield_value` 保存当前值并挂起；因为是无限序列，消费端要用 `take` 之类截断。（C++20 标准库**没有**内置 `generator`，需要自己实现或借助第三方库；C++23 才有了 `std::generator`。）

</details>
