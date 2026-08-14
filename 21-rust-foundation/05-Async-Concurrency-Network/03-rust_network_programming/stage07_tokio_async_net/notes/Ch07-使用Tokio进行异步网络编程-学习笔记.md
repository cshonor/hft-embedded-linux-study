# 第 7 章 — 使用 Tokio 进行异步网络编程（Asynchronous Network Programming Using Tokio）

> **按节学习（笔记 + demo）**：[本章学习笔记.md](../本章学习笔记.md) → `7.1-looking-into-the-future.md` … `7.3-conclusion.md`  
> 下文为**全章贯通稿**。

> 《Network Programming with Rust》· 全书对你价值最高的一章  
> 阶段目录：[stage07_tokio_async_net](../README.md) · 阻塞对照：[Ch03](../../stage03_std_tcp_udp/notes/Ch03-使用Rust进行TCP和UDP编程-学习笔记.md) · **主线笔记**：仓库 [`02-async_tokio/`](../../../02-async_tokio/)

本章讲解 **Future + 事件循环** 驱动的异步模型，以及 **`tokio`** 生态在网络编程中的用法。

---

## 从阻塞到异步（为何需要本章）

| 模型 | 等待 I/O 时 | 典型 API |
|------|-------------|----------|
| **阻塞**（Ch3 `std::net`） | 线程卡住 | `read` / `accept` 阻塞 |
| **异步**（本章） | 线程去干别的 | `read().await` 未就绪则 **Pending** |

目标：**单线程（或少量线程）上挂住大量连接**，避免「每连接一线程」的内存与调度开销。

底层栈回顾：

```text
tokio（运行时 + tokio::net）
  → mio（Poll / epoll，Ch3 已介绍）
  → 非阻塞 fd + 事件循环
```

---

## 1. Futures 抽象与基础

书中基于 **`futures`** crate；现代 Rust 异步以 **`std::task::Poll`** + **`Future`** trait 为核心（`futures` 仍提供组合子工具）。

### Future 是什么

**Future**：表示**尚未完成**、将来会得到一个值的计算（占位符 / 期约）。

```rust
// 概念：async fn 编译后就是实现 Future 的状态机
async fn fetch() -> String { ... }
```

### `poll` 与状态（书中 → 现代对照）

| 书中（较旧） | 现代 `Poll<T>` | 含义 |
|--------------|----------------|------|
| `Async::Ready(v)` | `Poll::Ready(v)` | 完成，得到 `v` |
| `Async::NotReady` | `Poll::Pending` | 未完成，稍后再 poll |
| `Err(e)` | `Poll::Ready(Err(e))` 或 Future 的 `Output = Result<…>` | 错误 |

**惰性**：Future **不会自己跑**；必须由 **执行器（Executor）** 在事件循环里反复 `poll`，直到 `Ready`。

### 三种执行方式（书中）

| 方式 | 行为 | 适用 |
|------|------|------|
| 当前线程阻塞执行 | 卡住线程直到完成 | 测试、桥接阻塞代码 |
| 线程池（如 `futures-cpupool`） | CPU 密集任务 offload | 计算，非 I/O 主路径 |
| **事件循环 / Reactor** | I/O Pending 时调度其他任务 | **网络服务主流** |

**Tokio** = Reactor + 线程池（可选 `spawn_blocking`）+ `tokio::net`。

### 组合子（Chaining / Combinators）

把多个异步步骤拼成一条链，避免回调地狱：

| 组合子（`futures` / async） | 作用 |
|---------------------------|------|
| `and_then` / `.await` 顺序 | 上一步完成再执行下一步 |
| `map` | 变换结果类型 |
| `select!` / `select_ok` | 多路竞争，谁先完成用谁 |
| `join!` | 多个 future 全部完成 |

现代写法以 **`async/await`** 为主，组合子仍用于 `select!`、`join!` 等宏场景。

```rust
// 现代等价
async fn work() -> Result<(), Error> {
    let a = step1().await?;
    let b = step2(a).await?;
    Ok(())
}
```

**仓库对照**：[`async_tokio/ch01_async_intro/`](../../../02-async_tokio/ch01_async_intro/) · [`ch02_futures`](../../../02-async_tokio/) 等。

---

## 2. Streams 与 Sinks

### Stream：异步版 Iterator

**Stream**：一系列**异步产生**的项；类似 `Iterator`，但 `next().await` 才拿下一项。

| Iterator | Stream |
|----------|--------|
| `next()` → `Option<Item>` | `poll_next` / `.next().await` → `Option<Item>` |
| `None` 结束 | `None` 表示流结束 |

用途：TCP 上连续读帧、WebSocket 消息、定时器 tick。

```rust
// tokio 中常见：while let Some(item) = stream.next().await { ... }
```

### Sink：异步版「写入端」

**Sink**：异步**接收**一系列项并消化（发到 socket、写到 channel）。

```text
Stream  ←── 读路径（入站数据）
Sink    ──→  写路径（出站数据）
```

套接字常拆成 **`Stream + Sink`** 或 **`AsyncRead + AsyncWrite`**（Tokio 扩展 trait）。

**与 Ch4**：Codec 解码后 often 得到 `Stream<Item = Frame>`。

---

## 3. 异步同步原语

标准库 `std::sync::Mutex`、`std::sync::mpsc` 在 async 里 **`.lock().await` 会阻塞线程**（或死锁风险），需要 **异步感知** 的类型。

| 原语 | 说明 |
|------|------|
| **`oneshot`** | 单次发送/接收，常用于「完成通知」 |
| **`mpsc`（async）** | 多生产者单消费者，在 task 间传消息 |
| **`BiLock`**（书中） | 两个 future 间仲裁共享状态，类似 async `Mutex` |

现代 Tokio 还提供：

- **`tokio::sync::Mutex`** — 持锁期间让出线程（`.lock().await`）  
- **`watch` / `broadcast`** — 状态广播  

**原则**：在 `.await` 点之间尽量**少持锁**；优先 **message passing**（`mpsc`）划分任务边界。

**仓库对照**：[`async_tokio/ch06`](../../../02-async_tokio/) 消息传递 · [`atomic`](../../../01-atomic/) 阻塞版 `mpsc` 对比学习。

---

## 4. Tokio 网络栈架构

### 底层：`mio` + `futures`

- **`mio`**：注册 fd，**`Poll::poll`** 等待可读/可写（Ch3 第三方节）。  
- **`futures` / executor**：把就绪事件转化为 **task wake → 再 poll Future**。

### 书中三层：`tokio-proto`（历史架构，概念仍有用）

| 层 | 职责 | 现代近似 |
|----|------|----------|
| **Codec** | 字节流 ↔ **帧**（`Encoder` / `Decoder`） | `tokio_util::codec::Framed`、`bytes::BytesMut` |
| **Protocol** | 把 Codec 绑到事件循环，定义请求-响应/多路复用/流式 | 自定义 state machine、`hyper` 服务协议 |
| **Service** | 业务：请求 → 异步响应（一个 Future） | `async fn handler(req) -> Response` |

```text
Socket 字节
   ↓ Codec（分帧）
   ↓ Protocol（状态机、多路复用）
   ↓ Service（Collatz / 业务逻辑）
   ↓ 响应编码回去
```

**`tokio-core` Reactor**：书中对 **事件循环核心** 的称呼 → 现为 **`tokio::runtime::Runtime`**（`#[tokio::main]`）。

### 最小现代服务器骨架

```rust
use tokio::net::TcpListener;
use tokio::io::{AsyncReadExt, AsyncWriteExt};

#[tokio::main]
async fn main() -> std::io::Result<()> {
    let listener = TcpListener::bind("127.0.0.1:8080").await?;
    loop {
        let (mut socket, _) = listener.accept().await?;
        tokio::spawn(async move {
            let mut buf = [0u8; 1024];
            let n = socket.read(&mut buf).await.unwrap_or(0);
            if n == 0 { return; }
            let _ = socket.write_all(&buf[..n]).await;
        });
    }
}
```

与 Ch3 对比：**`accept().await` 不阻塞线程池里其他任务**。

---

## 5. 高级网络通信模式（Collatz 服务器示例）

书中用多版 **Collatz 序列计算服务器** 演示协议演进。

### 套接字多路复用（Multiplexing）

| 问题 | 多路复用做法 |
|------|--------------|
| 多个请求同时在飞 | 每请求分配 **`RequestId`** |
| 响应顺序不必与到达顺序一致 | 处理完即回写，短任务可先返回 |
| 效果 | 提高并发下**响应性**与吞吐 |

```text
Client A  req id=1 ─┐
Client B  req id=2 ─┼→ Server 并行处理
Client C  req id=3 ─┘
         ← resp 2 可能先于 resp 1 返回
```

类似 **HTTP/2 流 ID**、RPC 并发 call id 的思想。

### 流式协议（Streaming）

| 场景 | 做法 |
|------|------|
| 大 Body 未收全 | 先读 **Header** → 立刻路由/鉴权/开管道 |
| 边收边处理 | 不必等整个 Body 落盘 |

```text
[ Header 已完整 ] → 可先 dispatch
[ Body  仍在传输 ] → Stream 继续 read
```

**与 Ch4 nom / Codec**：Header 定长或长度前缀 → Decoder 先吐一帧 Header。

---

## 6. Tokio 生态系统（书中列举 → 现代对照）

| 书中 crate | 作用 | 现代常见替代 |
|------------|------|--------------|
| **`tokio-io`** | 异步 Read/Write 工具 | `tokio::io`、`AsyncReadExt` |
| **`tokio-curl`** | 异步 HTTP 客户端 | **`reqwest`**（内置 Tokio） |
| **`tokio-tls`** | 非阻塞 TLS | **`tokio-rustls`**、`native-tls` + tokio |

另有（书外但常一起用）：

| crate | 作用 |
|-------|------|
| **hyper** | HTTP（Ch6） |
| **tonic** | gRPC（Ch5） |
| **tower** | 中间件、Service trait |

**TLS 进阶**：[stage08_network_security](../../stage08_network_security/README.md)。

---

## 学习路线：本书 Ch7 ↔ 本仓库 `02-async_tokio/`

| 本书主题 | 建议对照目录 |
|----------|--------------|
| Future / async 入门 | `async_tokio/ch01_async_intro/` |
| Future 组合、并发 | `ch02`～`ch03` |
| Channel / 共享状态 | `ch06_async_channels` |
| Tokio 运行时、任务 | `ch07_tokio_*` |
| 优雅关闭 | `ch07_tokio_graceful_shutdown/` |
| 测试与竞态 | `ch11_async_testing_debugging/` |

根 workspace：`Cargo.toml` 中 `ch07_*`、`ch01_reqwest_join` 等 **[[example]]**。

---

## 本章自检（Stage 07）

- [ ] 能解释 `Poll::Ready` vs `Poll::Pending`  
- [ ] 能对比阻塞 `read` 与 `read().await`  
- [ ] 能说明 Stream / Sink 与 Iterator 的类比  
- [ ] 知道为何 async 里要用 `tokio::sync::mpsc` 而非阻塞 `std::sync::mpsc`  
- [ ] 能画出 Codec → Protocol → Service 三层  
- [ ] 能解释多路复用 RequestId 与流式 Header/Body 处理  

---

## 建议 Demo（本 stage）

- [ ] 异步 echo（`tokio::spawn` + 多连接）  
- [ ] `tokio::select!` + `timeout` 管理连接生命周期  
- [ ] Framed 长度前缀 + JSON（接 Ch4）  
- [ ] graceful shutdown（对照 `async_tokio` ch07）  

---

## 下一步

| 方向 | 路径 |
|------|------|
| TLS | [stage08](../../stage08_network_security/README.md) |
| Future 底层 | [stage09](../../stage09_appendix_deep/README.md) |
| 并发底层 | [`01-atomic/`](../../../01-atomic/) |
