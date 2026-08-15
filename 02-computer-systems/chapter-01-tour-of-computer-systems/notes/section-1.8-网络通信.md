## 1.8 系统之间利用网络通信

### 从单机到分布式

- 现代程序常 **跨机器通信** — 浏览器/服务器、微服务、**交易所行情与订单网关**
- 逻辑上仍是 **进程通过 socket 读写字节流**；物理上经 **NIC → 链路 → 协议栈**

```
应用 write()
  → 内核 TCP/IP 栈（或 UDP）
  → 网卡 DMA
  → 线缆/光纤 → 对端 NIC → 对端内核 → 对端 read()
```

### 与 hello 的对比

- 本地 `hello`：`write(1, …)` 到终端
- 网络服务：`write(socket, …)` 经协议栈到网卡 — **延迟与抖动来源更多**

| 因素 | 影响 |
|------|------|
| RTT | 物理距离、交换机跳数 |
| 协议栈 | 拷贝次数、系统调用、中断/NAPI |
| 缓冲 | Nagle、发送/接收窗口 |
| 共置 | 同机房 vs 跨地域 — HFT 核心竞争力之一 |

**HFT 阅读链：**

```
Ch 1.8 概念（本节）
  → CSAPP Ch 11 网络编程
  → UNP Vol.1
  → 内核网络 Rosen
  → DPDK 旁路
  → 12-HFT ch06/ch10
```

→ [Ch 11 网络编程](../../chapter-11-network-programming/) · [16-Systems-Performance Ch 10 网络](../../../15-systems-performance/chapter-10-network/)

### 自测题

<details>
<summary>1. 网络通信在 CSAPP 中属于哪个层次？和后续章节什么关系？</summary>

Ch1.8 是概览层介绍，将网络抽象为「另一台机器的进程通过 socket 通信」。详细在 Ch10（系统级 I/O：fd/read/write）和 Ch11（网络编程：socket API、客户端-服务器模型）。HFT 延伸 → DPDK/onload 绕过内核协议栈。

</details>

<details>
<summary>2. 为什么 HFT 不直接用 CSAPP 的 socket 模型？</summary>

CSAPP 的 socket + read/write 路径经过内核协议栈（syscall 开销、数据拷贝、上下文切换），延迟在微秒级。HFT 用 **DPDK/onload** 绕过内核——用户态网卡驱动 + 零拷贝 + 轮询模式，延迟降到亚微秒级。但 CSAPP 的模型是理解网络编程的基础。

</details>


---

← [本章导读](../README.md)
