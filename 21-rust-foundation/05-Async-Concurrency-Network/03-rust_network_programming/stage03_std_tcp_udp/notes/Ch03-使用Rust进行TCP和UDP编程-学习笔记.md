# 第 3 章 — 使用 Rust 进行 TCP 和 UDP 编程（TCP and UDP Using Rust）

> **按节学习（笔记 + demo）**：[本章学习笔记.md](../本章学习笔记.md) → `3.1-simple-tcp-server-client.md` … `3.5-summary.md`  
> 下文为**全章贯通稿**；扩展时优先改各节 `.md` 与 demo。

> 《Network Programming with Rust》· 把 Ch1 理论落到 `std::net`  
> 阶段目录：[stage03_std_tcp_udp](../README.md) · 理论：[Ch01](../../stage01_network_basic/notes/Ch01-客户端服务器网络简介-学习笔记.md) · 异步进阶：[stage07](../../stage07_tokio_async_net/README.md)

本章用标准库 **`std::net`** 与部分社区 crate 编写真实的 TCP/UDP 服务器与客户端。

---

## 1. 简单的多线程 TCP 服务器与客户端

### TCP 服务器：`TcpListener`

| 步骤 | API / 做法 |
|------|------------|
| 监听 | `TcpListener::bind("127.0.0.1:8080")?` 绑定本地 IP + 端口 |
| 接受连接 | `listener.incoming()` 或 `accept()` → 得到 **`TcpStream`** |
| 并发模型 | 每接受一个客户端，**`thread::spawn`** 开工作线程处理该 `TcpStream`，避免单线程在 `accept`/`read` 上阻塞全体客户端 |

```rust
use std::io::{Read, Write};
use std::net::{TcpListener, TcpStream};
use std::thread;

fn handle_client(mut stream: TcpStream) -> std::io::Result<()> {
    let mut buf = [0u8; 1024];
    let n = stream.read(&mut buf)?;
    stream.write_all(&buf[..n])?;
    Ok(())
}

fn main() -> std::io::Result<()> {
    let listener = TcpListener::bind("127.0.0.1:8080")?;
    for stream in listener.incoming() {
        let stream = stream?;
        thread::spawn(move || {
            let _ = handle_client(stream);
        });
    }
    Ok(())
}
```

**要点**：

- `bind` 之后进入 **LISTEN**（对应 Ch1 TCP 状态机）。  
- `incoming()` 迭代器里每次 `accept` 等价于 C 的 `accept`，得到已建立连接的流。  
- 线程里要 **`move`** 拿走 `TcpStream` 所有权。  
- 生产环境还需：优雅退出、连接上限、错误日志、`join` 或线程池等（本书先建立最小模型）。

### TCP 客户端：`TcpStream`

| 步骤 | API |
|------|-----|
| 连接 | `TcpStream::connect("127.0.0.1:8080")?`（域名也可，见 §3 DNS） |
| 读写 | `Read` / `Write`（`read`、`write_all`） |
| 缓冲 | **`BufReader` / `BufWriter`** 减少系统调用次数，按行或按块读更方便 |

```rust
use std::io::{Read, Write};
use std::net::TcpStream;

fn main() -> std::io::Result<()> {
    let mut stream = TcpStream::connect("127.0.0.1:8080")?;
    stream.write_all(b"hello")?;
    let mut buf = [0u8; 1024];
    let n = stream.read(&mut buf)?;
    println!("{}", String::from_utf8_lossy(&buf[..n]));
    Ok(())
}
```

### 超时：`Duration` 与 `set_read_timeout`

网络延迟或对端无响应时，阻塞 `read` 可能一直卡住。

```rust
use std::net::TcpStream;
use std::time::Duration;

let stream = TcpStream::connect("127.0.0.1:8080")?;
stream.set_read_timeout(Some(Duration::from_secs(5)))?;
// 超时后 read 返回 Err，kind 常为 TimedOut
```

- 还可设置 **`set_write_timeout`**、**`set_read_timeout`** 在已连接的流上。  
- 连接阶段超时需配合非阻塞 + `connect` 或第三方 crate；书上以读超时为主。

**对照**：[`atomic/Chapter-01`](../../../01-atomic/Chapter-01-Rust-Concurrency-Basics/1.1-threads-in-rust/1.1-threads-in-rust.md) 线程模型 · 异步版见 [stage07](../../stage07_tokio_async_net/README.md)。

---

## 2. 简单的 UDP 服务器与客户端

UDP **无连接**，没有 TCP 的「字节流」语义，而是**独立数据报（datagram）**。

### 基础：`UdpSocket`

| 角色 | 典型调用 |
|------|----------|
| 服务端 | `UdpSocket::bind("0.0.0.0:9000")?` |
| 客户端 | 同样 `bind` 本地端口（或由 OS 分配 ephemeral 端口） |
| 收 | **`recv_from(&mut buf)`** → `(usize, SocketAddr)` 字节数 + 来源地址 |
| 发 | **`send_to(&buf, target_addr)`** → 发往指定地址 |

```rust
use std::net::UdpSocket;

let socket = UdpSocket::bind("127.0.0.1:9000")?;
let mut buf = [0u8; 1500];
let (len, src) = socket.recv_from(&mut buf)?;
socket.send_to(&buf[..len], src)?;
```

**与 TCP 对比**：

| | TCP | UDP |
|---|-----|-----|
| 连接 | `connect` / `accept` | 无；每包带地址 |
| 边界 | 流（需自己分帧） | **报文边界保留** |
| 可靠 | 协议保证 | 不保证 |

### UDP 多播（Multicasting）

- 多播地址常落在 **239.0.0.0/8**（IPv4），如 **`239.0.0.1`**。  
- **`join_multicast_v4(multicast_ip, interface_ip)`** 把套接字加入多播组，即可一对多接收。  
- 还需注意 OS 防火墙、网卡与 TTL；书上侧重 API 用法。

**场景**：局域网服务发现、流媒体（常配合 RTP）、日志汇聚等。

---

## 3. `std::net` 中的其他实用工具

### `IpAddr` 与 `SocketAddr`

| 类型 | 含义 |
|------|------|
| **`IpAddr`** | 枚举：`V4(Ipv4Addr)` / `V6(Ipv6Addr)` |
| **`SocketAddr`** | `SocketAddrV4` / `SocketAddrV6`：**IP + 端口** |

常用方法（便于校验与分支）：

- `is_loopback()` — `127.0.0.1` / `::1`  
- `is_global()` — 是否全球单播  
- `is_ipv4()` / `is_ipv6()`

解析字符串：

```rust
use std::net::SocketAddr;
let addr: SocketAddr = "192.168.1.10:8080".parse()?;
```

### DNS 解析

书中示例：**`std::net::lookup_host`**（历史上为 **nightly + `#![feature(lookup_host)]`**），底层常走 C 的 **`getaddrinfo`**，一般只得到 **A / AAAA** 记录。

**稳定替代（日常推荐）**：

```rust
// ToSocketAddrs：connect 时自动解析
use std::net::TcpStream;
let _ = TcpStream::connect("example.com:80")?;
```

需要更多记录类型（MX、NS、TXT）或异步解析 → 见 §4 **`trust-dns`**（现生态中常见 **`hickory-dns`** 等 fork/继任）。

---

## 4. 网络编程相关的第三方 Crates

标准库适合「常见 TCP/UDP 应用」；细粒度控制、CIDR、链路层、完整 DNS 等需社区库。

| Crate | 弥补的能力 | 要点 |
|-------|------------|------|
| **`ipnetwork`** | **CIDR**（如 `192.168.1.0/24`） | 网段大小、判断 IP 是否属于某前缀 |
| **`mio`** | **非阻塞 I/O + 事件循环** | `Poll` 监控多 fd，**Token** 区分事件源；**Tokio 的底层基石** |
| **`libpnet`** | **数据链路层 / 原始抓包** | 从网卡读以太网帧，自解析 IPv4/TCP 头；标准库做不到 |
| **`trust-dns`** | **完整 DNS** | 任意记录类型；同步解析器 + 基于 Tokio 的异步解析器 |

### `mio` 与多线程模型（心智对照）

```text
多线程阻塞：每连接一线程 → 简单，连接数大时线程开销高
mio / epoll：单线程（或少量线程）事件循环 → 多连接复用，为 Tokio/async 铺路
```

学习路径：**本章阻塞 `std::net` → 理解 mio 在等什么 → stage07 Tokio**。

### `libpnet` 嗅探器（概念）

- 绕过常规 socket API，直接读**帧**。  
- 需管理员权限 / 能力（`CAP_NET_RAW` 等）。  
- 用于学习协议头、安全分析、自定义协议栈实验。

### 为何不用裸 `libc`？

可直接调 POSIX **`socket` / `bind` / `listen`** 等 C API，但：

- 易破坏 Rust **内存与类型安全**；  
- 更推荐 **`nix`** 等 **纯 Rust 封装** 的系统调用库，在 `unsafe` 边界集中审查。

---

## 本章自检（Stage 03）

- [ ] 能写出 `TcpListener::bind` + `incoming` + `thread::spawn` 处理客户端  
- [ ] 会用 `TcpStream` + `BufReader` 读写，并设置 `set_read_timeout`  
- [ ] 能说明 UDP 的 `recv_from` / `send_to` 与 TCP 流的区别  
- [ ] 知道 `SocketAddr` / `IpAddr` 的用途  
- [ ] 能解释 `mio` 相对「一线程一连接」解决了什么问题  
- [ ] 知道 CIDR / 全功能 DNS / 链路层抓包分别该找哪个 crate  

---

## 建议 Demo（本 stage）

在 `demo/` 下逐步实现（见 [stage03 README](../README.md)）：

- [ ] TCP 回显服务端 + 客户端  
- [ ] 换行或长度前缀分帧的一轮请求—响应  
- [ ] UDP 一发一收  
- [ ] （可选）`join_multicast_v4` 多播  

仓库对照：`async_tokio/ch10_dependency_free_async_server/`（偏 `std` 的异步/同步示例）。

---

## 下一步

| 方向 | 路径 |
|------|------|
| 序列化 / 协议体 | [stage04_serde_parse](../../stage04_serde_parse/README.md) |
| Tokio 异步网络 | [stage07_tokio_async_net](../../stage07_tokio_async_net/README.md) |
| TCP 状态机细节 | [Ch01 + TCP 笔记](../../stage01_network_basic/notes/TCP-传输层-完整笔记与背诵卡.md) |
