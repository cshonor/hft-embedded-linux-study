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

→ [Ch 11 网络编程](../../chapter-11-network-programming/) · [13-Systems-Performance Ch 10 网络](../../../13-Systems-Performance-2nd/chapter-10-network/)

---

← [本章导读](../README.md)
