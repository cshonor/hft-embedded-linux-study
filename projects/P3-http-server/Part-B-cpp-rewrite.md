# P3 Part B — C++ 重写版

> 同一个 HTTP server 用 Modern C++ 重写，亲手感受 RAII/模板/移动语义怎么让代码更安全又不损性能。

---

## 最小预备

| 瞄一眼 | 只要留下印象 |
|--------|-------------|
| [Effective Modern C++ ch01](../../04-cpp/M1-modern-cpp/01-Effective-Modern-C++) | auto/unique_ptr/move 语义 |
| [Cpp Concurrency ch02-03](../../04-cpp/M3-deep-principles/02-Cpp-Concurrency/ch02-managing-threads) | std::thread / mutex / lock_guard |
| [Cpp Concurrency ch08](../../04-cpp/M3-deep-principles/02-Cpp-Concurrency/ch08-designing-concurrent-code) | 线程池设计 |
| [CSAPP 12.3 线程](../../02-computer-systems/chapter-12-concurrent-programming/notes/section-12.3-基于线程的并发编程.md) | 线程基础概念 |

---

## Phase 1：RAII fd 封装（30 分钟）

### 做什么

用 RAII 封装 fd——析构自动 `close`，从语言层面消灭 fd 泄漏。

### 代码骨架

```cpp
// src/raii_fd.hpp
#pragma once
#include <unistd.h>
#include <stdexcept>

class Fd {
public:
    Fd() : fd_(-1) {}
    explicit Fd(int fd) : fd_(fd) {}
    ~Fd() { if (fd_ >= 0) ::close(fd_); }

    // 禁止拷贝（fd 不能共享所有权）
    Fd(const Fd&) = delete;
    Fd& operator=(const Fd&) = delete;

    // 允许移动（转移所有权）
    Fd(Fd&& other) noexcept : fd_(other.fd_) { other.fd_ = -1; }
    Fd& operator=(Fd&& other) noexcept {
        if (this != &other) {
            if (fd_ >= 0) ::close(fd_);
            fd_ = other.fd_;
            other.fd_ = -1;
        }
        return *this;
    }

    int get() const { return fd_; }
    int release() { int t = fd_; fd_ = -1; return t; }

    // 隐式转 int（方便传给系统调用）
    operator int() const { return fd_; }

private:
    int fd_;
};
```

### 分步实现

1. **写 `Fd` 类**：构造存 fd，析构 close，禁拷贝允许移动
2. **测试**：`{ Fd f(socket(...)); }` — 出作用域自动 close
3. **对比 C 版**：C 版每个 `return` 前都要 `close(fd)`，漏一个就泄漏；C++ 版析构兜底

### 为什么重要

这是 RAII 的核心价值——**资源获取即初始化，资源释放即析构**。`Fd`、`std::lock_guard`、`std::unique_ptr` 都是同一个模式。C 版靠程序员记得 free/close，C++ 版靠编译器保证。

### 常见坑

| 坑 | 症状 | 原因 |
|----|------|------|
| 忘了 `delete` 拷贝构造 | double close | 默认拷贝构造会复制 fd_ |
| `operator int()` 太宽松 | 意外隐式转换 | 可以改成 `explicit operator int()` |
| 移动后还用旧对象 | 访问 -1 | 移动后 fd_ = -1，调用系统调用返回 EBADF |

### 卡住翻哪篇笔记

| 卡住了… | 翻这里 |
|---------|--------|
| RAII / 三五零法则 | [CSAPP 13 拷贝控制](../../04-cpp/M0-entry-syntax/01-C++Primer/ch13-copy-control/) (C++Primer ch13 笔记) |
| move 语义 | [Effective Modern C++ item1-10](../../04-cpp/M1-modern-cpp/01-Effective-Modern-C++/) |
| unique_ptr | [Effective Modern C++ item18](../../04-cpp/M1-modern-cpp/01-Effective-Modern-C++/) |

---

## Phase 2：线程池模板（1 小时）

### 做什么

用 `std::function` + `std::mutex` + `std::condition_variable` 写一个泛型线程池，能接受任意可调用任务。

### 代码骨架

```cpp
// src/threadpool.hpp
#pragma once
#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <future>
#include <atomic>

class ThreadPool {
public:
    explicit ThreadPool(size_t num_threads) : stop_(false) {
        for (size_t i = 0; i < num_threads; ++i)
            workers_.emplace_back([this] { worker_loop(); });
    }

    ~ThreadPool() {
        { std::lock_guard<std::mutex> lock(mtx_); stop_ = true; }
        cv_.notify_all();
        for (auto &t : workers_) t.join();
    }

    // 提交任意任务，返回 future
    template <typename F, typename... Args>
    auto submit(F&& f, Args&&... args)
        -> std::future<decltype(f(args...))>
    {
        using RetType = decltype(f(args...));
        auto task = std::make_shared<std::packaged_task<RetType()>>(
            std::bind(std::forward<F>(f), std::forward<Args>(args)...)
        );
        std::future<RetType> result = task->get_future();
        {
            std::lock_guard<std::mutex> lock(mtx_);
            tasks_.emplace([task]() { (*task)(); });
        }
        cv_.notify_one();
        return result;
    }

private:
    void worker_loop() {
        for (;;) {
            std::function<void()> task;
            {
                std::unique_lock<std::mutex> lock(mtx_);
                cv_.wait(lock, [this] { return stop_ || !tasks_.empty(); });
                if (stop_ && tasks_.empty()) return;
                task = std::move(tasks_.front());
                tasks_.pop();
            }
            task();
        }
    }

    std::vector<std::thread> workers_;
    std::queue<std::function<void()>> tasks_;
    std::mutex mtx_;
    std::condition_variable cv_;
    bool stop_;
};
```

### 分步实现

1. **先写最简版**：`std::queue<std::function<void()>>` + mutex + cv + N 个 worker
2. **加 `submit` 模板**：用 `std::packaged_task` 包装任务，返回 `std::future`
3. **优雅关闭**：析构时 `stop_ = true`，notify_all，join 所有线程
4. **对比 C 版**：C 版任务队列只能存 `int fd`；C++ 版能存任意 lambda

### 常见坑

| 坑 | 症状 | 原因 |
|----|------|------|
| `std::function` 存引用 | 悬垂引用 | 用 `std::ref` 或值传递 |
| packaged_task 生命周期 | 段错误 | task 必须用 `shared_ptr` 延长生命周期 |
| 析构时 task 队列还有任务 | 丢任务 | 析构前 drain 或在析构里处理完剩余任务 |

### 卡住翻哪篇笔记

| 卡住了… | 翻这里 |
|---------|--------|
| std::thread 管理 | [Concurrency ch02](../../04-cpp/M3-deep-principles/02-Cpp-Concurrency/ch02-managing-threads/) |
| mutex/lock_guard | [Concurrency ch03](../../04-cpp/M3-deep-principles/02-Cpp-Concurrency/ch03-sharing-data/) |
| condition_variable | [Concurrency ch04](../../04-cpp/M3-deep-principles/02-Cpp-Concurrency/ch04-synchronizing-operations/) |
| 线程池设计 | [Concurrency ch08](../../04-cpp/M3-deep-principles/02-Cpp-Concurrency/ch08-designing-concurrent-code/) |

---

## Phase 3：HTTP 处理 + Modern C++ 特性（1 小时）

### 做什么

用 `std::string_view` 解析请求行，用 `std::optional` 处理可能失败的解析，用移动语义传递请求对象。

### 代码骨架

```cpp
// src/http_request.hpp
#pragma once
#include <string>
#include <string_view>
#include <optional>
#include <unordered_map>

struct HttpRequest {
    std::string method;
    std::string path;
    std::unordered_map<std::string, std::string> headers;
    std::string body;

    // 移动构造（零拷贝传递）
    HttpRequest(HttpRequest&&) = default;
    HttpRequest& operator=(HttpRequest&&) = default;
};

// 用 string_view 解析，不拷贝字符串
std::optional<HttpRequest> parse_request(std::string_view raw) {
    HttpRequest req;
    // 找请求行
    auto line_end = raw.find("\r\n");
    if (line_end == std::string_view::npos) return std::nullopt;

    std::string_view line = raw.substr(0, line_end);
    // GET /path HTTP/1.1
    auto sp1 = line.find(' ');
    auto sp2 = line.find(' ', sp1 + 1);
    if (sp1 == std::string_view::npos || sp2 == std::string_view::npos)
        return std::nullopt;

    req.method = std::string(line.substr(0, sp1));
    req.path = std::string(line.substr(sp1 + 1, sp2 - sp1 - 1));
    return req;
}

// 错误处理：optional 替代裸指针/错误码
std::string handle_request(const HttpRequest& req) {
    if (req.method == "GET") {
        if (req.path == "/") return "<h1>Hello from C++</h1>";
        // ...
    }
    return "404 Not Found";
}
```

### 分步实现

1. **`std::string_view`** 解析请求行：不分配内存，直接在原始 buffer 上切片
2. **`std::optional<HttpRequest>`** 返回解析结果：`nullopt` = 解析失败，不需要错误码
3. **移动语义**：`HttpRequest` 通过 `std::move` 在线程间传递，零拷贝
4. **对比 C 版**：C 版用 `char[]` + `strtok` + 全局错误码；C++ 版 `string_view` + `optional` + move

### 为什么重要

这三种特性是 C++ 相对 C 的核心优势：
- `string_view` = 零拷贝字符串操作（C 版只能用 `char*` + 手动算长度）
- `optional` = 类型安全的错误处理（C 版用返回值 + errno）
- move = 零拷贝所有权转移（C 版只能用指针 + 手动管理）

### 卡住翻哪篇笔记

| 卡住了… | 翻这里 |
|---------|--------|
| string_view | [Effective Modern C++ item17](../../04-cpp/M1-modern-cpp/01-Effective-Modern-C++/) |
| optional / variant | [C++Primer ch17](../../04-cpp/M0-entry-syntax/01-C++Primer/ch17-special-library-facilities/) |
| 移动语义 | [C++Primer ch13](../../04-cpp/M0-entry-syntax/01-C++Primer/ch13-copy-control/) |

---

## Phase 4：功能对齐 + 代码对比（1 小时）

### 做什么

让 C++ 版功能跟 C 版完全对齐，然后对比代码量、安全性、性能。

### 对比清单

| 维度 | C 版 | C++ 版 | 差异 |
|------|------|--------|------|
| fd 管理 | 手动 close，每个 return 前都要 | RAII 析构自动 close | C++ 消灭整类 bug |
| 线程池 | 固定 `int fd` 队列 | `std::function<void()>` 任意任务 | C++ 灵活得多 |
| 请求解析 | `char[]` + `strtok` + `sscanf` | `string_view` + `optional` | C++ 类型安全 |
| 错误处理 | 返回值 + errno + goto cleanup | `optional` / 异常 / RAII | C++ 无 goto |
| 内存管理 | `malloc/free` + 手动追踪 | `unique_ptr` / `vector` / RAII | C++ 零泄漏 |
| HTTP 响应 | `snprintf` 拼字符串 | `std::format` (C++20) 或流 | 可读性 |
| 代码行数 | ~400 行 | ~250 行 | C++ 更短 |
| 性能 | 基准 | 基准 ±5% | 几乎相同 |

### 测试

```bash
# 两版都压测
ab -n 10000 -c 100 http://localhost:8080/
# 对比 RPS 和 p99 延迟
```

← [P3 索引](./README.md) · [04-cpp 模块](../../04-cpp/)
