## 11.7 小结（原书）

> **Ch11 §11.7** · [章导读](../README.md) · 上节 [§11.6 ←](./section-11.6-综合TinyWebServer.md) · 下节 —

---

← [本章导读](../README.md)

---

### Ch11 全章要点

| 主题 | 核心概念 | HFT 关联 |
|------|----------|----------|
| §11.1 | C/S 模型、socket=fd | 行情客户端、订单网关 |
| §11.2 | 协议分层、LAN/WAN | 各层延迟优化 |
| §11.3 | IP+端口、DNS | 预解析 DNS，缓存 IP |
| §11.4 | socket API 全流程 | TCP_NODELAY、epoll、multicast |
| §11.5 | HTTP、静态/动态 | admin API 用 HTTP |
| §11.6 | Tiny Web Server | 迭代→并发演进 |

**一句话：** 网络编程 = socket API（socket→bind→listen→accept→read/write→close）+ 协议分层（应用/传输/网络/链路），HFT 用 TCP_NODELAY 降延迟、epoll 管多连接、UDP multicast 收行情。

### 常见陷阱
1. **socket API 顺序不能乱** — 客户端：socket→connect→read/write；服务器：socket→bind→listen→accept→read/write
2. **HFT 网络延迟优化是全栈的** — 不只应用层，链路层（NIC）、传输层（TCP 选项）都要管
3. **Tiny Web Server 是教学版** — 生产用 epoll reactor + 线程池，不能阻塞迭代

### 自测题

<details>
<summary>Q1: 客户端和服务器的 socket API 调用顺序分别是什么？</summary>

客户端：socket() → connect() → read/write() → close()。服务器：socket() → bind() → listen() → accept() → read/write() → close()。

</details>

<details>
<summary>Q2: HFT 网络编程的三个关键优化是什么？</summary>

1) TCP_NODELAY 禁 Nagle（降小包延迟）；2) epoll 单线程管多连接（避免线程切换）；3) UDP multicast 收行情（一对多，低延迟）。

</details>

<details>
<summary>Q3: socket API 中哪些调用可能阻塞？HFT 如何处理？</summary>

connect（TCP 握手）、accept（等待连接）、read（等待数据）都可能阻塞。HFT 用 O_NONBLOCK + epoll：所有 socket 设非阻塞，epoll 通知就绪事件，不阻塞等待。

</details>

<details>
<summary>Q4: 从 Tiny Web Server 到生产级网络服务，需要哪些改进？</summary>

1) 迭代→并发（线程池/epoll reactor）；2) 阻塞→非阻塞（O_NONBLOCK）；3) 单进程→多进程（SO_REUSEPORT）；4) 无监控→健康检查+限流+日志。

</details>

---

← [§11.6 ←](./section-11.6-综合TinyWebServer.md) · [本章导读](../README.md) · —
