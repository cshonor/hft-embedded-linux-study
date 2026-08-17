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
