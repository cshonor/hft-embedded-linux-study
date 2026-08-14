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
