# B.4 ASIO 与网络并发

> 附录 B · 上一节：[B.3 OpenMP 的定位](03-openmp.md) · 下一节：[B.5 消息传递 vs 共享内存](05-msg-vs-shared.md)

## 这节讲什么

ASIO（Asynchronous I/O）是 C++ 网络编程的事实标准——异步 IO + C++20 协程支持。本节讲 ASIO 的核心模型（proactor）、回调 vs 协程写法、以及它在 HFT 网关中的应用。

---

## 核心规则（代码+表格）

### ASIO 的 Proactor 模型

```
传统 Reactor（libevent）：
  事件就绪 → 通知用户 → 用户 read/write
  （同步 IO，非阻塞）

ASIO Proactor：
  发起 async_read → 完成后回调被调用
  （异步 IO，操作系统完成读操作后通知）
  
Linux: 用 epoll 模拟 proactor
Windows: 用 IOCP（原生 proactor）
```

### 回调写法（传统）

```cpp
#include <asio.hpp>

void read_handler(const asio::error_code& ec, std::size_t bytes) {
    if (!ec) {
        std::cout << "read " << bytes << " bytes\n";
    }
}

void start_read(asio::ip::tcp::socket& socket, char* buffer) {
    socket.async_read_some(asio::buffer(buffer, 1024), read_handler);
    // 异步发起读，完成后调用 read_handler
}

// 回调地狱：读→处理→写→读→...
void read_then_process_then_write(asio::ip::tcp::socket& socket) {
    socket.async_read_some(asio::buffer(buf), [&](ec, n){
        process(buf, n);
        socket.async_write_some(asio::buffer(result), [&](ec, n){
            socket.async_read_some(asio::buffer(buf), [&](ec, n){
                // 无限嵌套...
            });
        });
    });
}
```

### 协程写法（C++20，ASIO 独立版）

```cpp
#include <asio.hpp>
using asio::awaitable;

// 线性写法，无嵌套
awaitable<void> session(asio::ip::tcp::socket socket) {
    char buf[1024];
    for (;;) {
        auto n = co_await socket.async_read_some(
            asio::buffer(buf), asio::use_awaitable);
        auto result = process(buf, n);
        co_await socket.async_write_some(
            asio::buffer(result), asio::use_awaitable);
    }
}

// 启动协程
asio::co_spawn(io_context, session(std::move(socket)), asio::detached);
```

### ASIO 核心组件

| 组件 | 作用 |
|------|------|
| `io_context` | 事件循环，所有异步操作在此运行 |
| `tcp::socket` / `udp::socket` | 网络 socket |
| `tcp::acceptor` | 服务端 accept |
| `steady_timer` | 定时器 |
| `buffer()` | 缓冲区封装 |
| `use_awaitable` | 协程适配器（C++20） |

### 服务端示例

```cpp
awaitable<void> listen(asio::io_context& ctx, short port) {
    auto executor = co_await asio::this_coro::executor;
    asio::ip::tcp::acceptor acceptor(executor,
        {asio::ip::tcp::v4(), port});
    for (;;) {
        auto socket = co_await acceptor.async_accept(asio::use_awaitable);
        asio::co_spawn(executor, session(std::move(socket)), asio::detached);
        // 每个连接一个协程
    }
}

int main() {
    asio::io_context ctx;
    asio::co_spawn(ctx, listen(ctx, 8080), asio::detached);
    ctx.run();  // 事件循环
}
```

---

## 新手要点（和 C 的区别）

- **C 程序员可能用 libevent/libuv**：C 的异步 IO 通常用 libevent（Reactor）或 libuv（Node.js 底层）。ASIO 是 C++ 生态的等价物，但模型不同（Proactor）。
- **Proactor vs Reactor**：C 程序员可能熟悉 Reactor（事件就绪通知）——ASIO 的 Proactor 是"发起操作，完成后通知"，抽象层次更高。两者在 Linux 上底层都用 epoll，但 ASIO 封装更高级。
- **协程是 C++20 的杀手锏**：C 的异步 IO 只能用回调（回调地狱）。C++20 的协程让异步代码"看起来像同步"——这是 C++ 相比 C 的巨大优势。
- **ASIO 独立版 vs Boost.Asio**：ASIO 原本是 Boost 的一部分（Boost.Asio），现在有独立版（standalone asio），不依赖 Boost。HFT 可以用独立版减少依赖。

---

## HFT 关联

- **HFT 网关用 ASIO**：HFT 的 TCP 网关（连接交易所行情/订单接口）通常用 ASIO——异步 IO + 协程，处理多连接高效。
- **HFT 行情用 UDP 而非 TCP**：HFT 的组播行情用 UDP（`udp::socket`）——ASIO 支持 UDP，但 HFT 的极致性能通常用 DPDK 绕过内核，不用 ASIO。
- **ASIO 用于管理面**：HFT 的管理面（监控、配置、日志上报）用 ASIO + TCP——比热路径的延迟要求低，ASIO 的协程让代码清晰。
- **`io_context` 单线程 vs 多线程**：HFT 通常用单线程 `io_context`（一个线程跑事件循环），避免锁——如果要多线程，用多 `io_context` 而非多线程跑同一个。

---

## 自测题

1. ASIO 的 Proactor 模型和传统 Reactor 有什么区别？
2. 回调写法和协程写法有什么区别？为什么协程更好？
3. ASIO 的 `io_context` 是什么？它的作用是什么？
4. C++20 协程如何让异步 IO 代码"看起来像同步"？
5. HFT 的哪些部分用 ASIO？热路径行情用 ASIO 吗？

---

## 参考与延伸

- 下一节：[B.5 消息传递 vs 共享内存](05-msg-vs-shared.md)
- 上一节：[B.3 OpenMP 的定位](03-openmp.md)
- 回到：[附录 B](README.md)
