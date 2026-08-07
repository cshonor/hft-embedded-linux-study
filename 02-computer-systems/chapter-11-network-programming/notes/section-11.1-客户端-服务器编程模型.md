## 11.1 客户端-服务器编程模型

> **Ch11 §11.1** · [章导读](../README.md) · 上节 — · 下节 [§11.2 →](./section-11.2-网络.md)

---

```
客户端进程  ←——网络——→  服务器进程
  发起连接              监听、accept、服务
```

- **套接字 (socket)** — 连接的端点，本质是 **fd**（→ [Ch 10](../../chapter-10-system-io/)）
- HFT：**行情客户端** 连交易所 feed；**订单网关** 作 TCP 客户端连券商

---

### 常见陷阱
1. **套接字本质是 fd** — socket 返回文件描述符，可用 read/write/close 操作，和普通文件统一接口
2. **客户端主动 connect，服务器被动 accept** — 角色不同，API 调用顺序也不同
3. **HFT 中行情客户端和订单网关角色不同** — 行情通常是 UDP multicast 接收方，订单是 TCP 客户端连券商

### 自测题

<details>
<summary>Q1: 客户端-服务器模型中，谁是主动方？谁是被动方？</summary>

客户端主动发起连接（connect），服务器被动监听（listen）并接受连接（accept）。服务器先运行，等待客户端连接。

</details>

<details>
<summary>Q2: 套接字（socket）在 Linux 中是什么？和文件描述符的关系？</summary>

套接字是一种文件描述符（fd）。socket() 返回一个 int fd，可以用 read/write/close 操作，与普通文件统一接口。内核为 socket fd 维护发送/接收缓冲区。

</details>

<details>
<summary>Q3: HFT 中常见的客户端-服务器角色有哪些？</summary>

行情客户端连交易所 feed（UDP multicast 或 TCP）；订单网关作为 TCP 客户端连券商/交易所；风控/监控 HTTP admin API 作为 Web 服务器。

</details>

<details>
<summary>Q4: 为什么说 socket 统一了网络和文件 I/O？</summary>

Linux 一切皆文件。socket fd 和文件 fd 共享同一套系统调用（read/write/close），差异由内核处理。select/epoll 可同时监听文件和 socket fd。

</details>

---

← — · [本章导读](../README.md) · [§11.2 →](./section-11.2-网络.md)
