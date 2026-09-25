# std::jthread

## jthread vs thread

```cpp
#include <thread>

// C++11 std::thread：析构时如果 joinable 会 std::terminate
{
    std::thread t([] { /* ... */ });
    // 忘记 t.join() 或 t.detach() → 析构时 terminate！
}

// C++20 std::jthread：RAII 自动 join
{
    std::jthread t([] { /* ... */ });
    // 析构时自动 join——不用手动管理
}
```

## 自动取消：stop_token

```cpp
// jthread 自带停止机制
std::jthread t([](std::stop_token st) {
    while (!st.stop_requested()) {
        // 工作循环
        do_work();
        std::this_thread::sleep_for(10ms);
    }
});

// 外部请求停止
t.request_stop();
// t 析构时自动 join

// 或手动
t.request_stop();
t.join();
```

## stop_token 详解

```cpp
// stop_source：控制停止
// stop_token：查询停止状态
// stop_callback：停止时回调

std::jthread t([](std::stop_token st) {
    // 注册回调：停止时执行
    std::stop_callback cb(st, [] {
        std::cout << "Stopping...\n";
    });

    while (!st.stop_requested()) {
        // 工作中...
    }
    // 退出循环后清理
});

// 外部停止
std::this_thread::sleep_for(1s);
t.request_stop();  // 触发回调 + 设置 stop_requested
// jthread 析构自动 join
```

## 与 condition_variable 配合

```cpp
// C++20：condition_variable 支持 stop_token
std::jthread t([](std::stop_token st) {
    std::mutex mtx;
    std::condition_variable_any cv;
    std::unique_lock lk(mtx);

    // 等待停止或超时
    cv.wait_for(lk, 100ms, st.get_stop_token(),
        [] { return false; });  // 超时或停止时返回

    // 或：
    while (!st.stop_requested()) {
        cv.wait_for(lk, 100ms);
        // ...
    }
});
```

## 实际应用

```cpp
// HFT：行情处理线程，可优雅停止
class MarketDataHandler {
    std::jthread worker;
public:
    void start() {
        worker = std::jthread([this](std::stop_token st) {
            while (!st.stop_requested()) {
                auto tick = recv_tick();
                if (tick) process(*tick);
            }
        });
    }
    void stop() {
        worker.request_stop();  // 通知线程停止
        // jthread 析构自动 join
    }
};
```

## 自测题

1. `jthread` 和 `thread` 的主要区别？
2. `thread` 析构时如果 joinable 会怎样？`jthread` 呢？
3. `stop_token` 的作用是什么？如何检测停止请求？
4. `stop_callback` 做什么？
5. HFT 行情处理线程如何用 `jthread` 实现优雅停止？

<details>
<summary>参考答案</summary>

1. 两点核心区别：
   1. **RAII 自动 join**：`jthread` 析构时若仍 joinable，会先请求停止再 `join()`，不会像 `thread` 那样直接终止程序。
   2. **内建协作式取消**：`jthread` 自带 `stop_source`/`stop_token` 机制，提供 `request_stop()`、`get_stop_token()`；并且若可调用物的第一个参数类型是 `std::stop_token`，构造时会自动把自己那个 token 传进去。
其余 API（`join`/`detach`/`get_id` 等）与 `thread` 一致。
2. `std::thread` 析构时若处于 joinable 状态 → 调用 **`std::terminate()`**，整个程序中止（这是 C++ 里最经典的坑之一）。
`std::jthread` 析构时若处于 joinable 状态 → 先 `request_stop()`，再 `join()`，安全收尾。
3. `std::stop_token` 是一个**只读的停止请求句柄**：线程代码用它查询"外部是否已请求我停止"。
检测方式是**主动轮询**（协作式取消，不是强制杀线程）：
```cpp
std::jthread t([](std::stop_token st) {   // token 自动注入
    while (!st.stop_requested()) {
        // 干活
    }
});
t.request_stop();   // 外部请求停止
```
也可以从 `jthread::get_stop_token()` 拿到它传给别的函数。配套的 `std::condition_variable_any` 的 `wait*` 重载接受 `stop_token`，能在请求停止时**立即唤醒**等待中的线程。
4. `std::stop_callback` 注册一个回调：当关联的 `stop_token` 被 `request_stop()` 时，该回调**立即在调用 `request_stop()` 的那个线程上同步执行**。
典型用途是"打断阻塞等待"——比如线程正阻塞在 `condition_variable` 上，回调里 `cv.notify_all()` 把它叫醒，线程再检查 `stop_requested()` 后退出，从而不必等超时就能快速停止。
回调在析构时（若尚未触发）自动注销，因此回调对象必须活得比它捕获的引用/句柄所涉及的对象短。
5. ```cpp
class MarketDataHandler {
    std::jthread worker;
public:
    void start() {
        worker = std::jthread([this](std::stop_token st) {
            while (!st.stop_requested()) {          // 每圈轮询
                if (auto tick = recv_tick()) process(*tick);
            }
        });
    }
    void stop() { worker.request_stop(); }          // 通知停止
    // jthread 析构时自动 request_stop() + join()
};
```
要点：① 循环条件是 `!st.stop_requested()`，让停止是**协作**的；② `stop()` 只发请求，不必（也不该）手动 join；③ 若线程会阻塞在条件变量上，用 `std::condition_variable_any` + `stop_token` 的 `wait` 重载，或配 `stop_callback` 做 notify，保证停止延迟可控；④ 收尾逻辑（flush、统计）放在循环之后。

</details>
