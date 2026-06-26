## 10.1–10.3 核心概念与延迟指标

### 基础术语

| 术语 | 含义 | HFT |
|------|------|-----|
| **Throughput** | 实际达到的数据率 | 行情 MB/s、订单 msg/s |
| **Bandwidth** | 链路/网卡理论上限 | 10G/25G/100G 协商速率 |
| **Socket** | 网络 I/O 端点 | TCP 发单、UDP 组播 |
| **MTU** | 最大传输单元（以太网常 1500） | 分片增延迟 — 路径 MTU _discovery |
| **Jumbo Frames** | MTU ~9000 | 共置 LAN 常用；跨 WAN 慎用 |

→ [07-TCP/IP](../../../12-TCP-IP-Illustrated-Vol1/) · [08-UNP](../../../11-UNP-Vol1/)

### 多维度网络延迟

| 延迟类型 | 包含什么 | 测量 |
|----------|----------|------|
| **DNS** | 名称解析 | `dig` 计时 |
| **Ping (ICMP)** | 网络往返（非业务路径） | `ping` — 粗筛连通 |
| **Connection** | TCP 三次握手 + **SYN 重传** | `ss`、BPF `tcpconnect` |
| **TTFB** | 请求 → **首字节**（含服务端 think time） | 应用层 span |
| **RTT** | 往返时间 | `ss -ti` 内 `rtt` |
| **Connection lifetime** | 建立 → 关闭 | BPF **`tcplife`** |

**HFT 延迟分解示例（TCP 发单）：**

```
signal_ts → encode → send() → 内核 TCP → NIC → wire RTT → 交易所 ACK
              ↑           ↑                    ↑
           TTFB 本地    Connection/队列      RTT（共置仍非零）
```

→ Ch 2 [延迟分解](../../chapter-02-methodologies/) · [12-HFT ch10](../../../15-HFT-Low-Latency-Practice/chapter-10-延迟测量与基准压测/)

### 缓冲与积压（Buffering & Backlog）

**Bufferbloat：**

- 交换机/路由器 **过大缓冲** → 排队延迟膨胀 — 吞吐高但 **latency 差**（与 HFT 目标相反）。
- 共置低延迟网络仍要关注 **交换机 buffer 配置**。

**TCP 连接积压：**

```
Client SYN → 内核 SYN queue / accept queue → listen socket backlog 满
                    ↓
              丢 SYN / 丢 ACK → 客户端重传 → 连接超时
```

| 队列 | 作用 |
|------|------|
| **SYN queue** | 半连接（SYN_RECV） |
| **Accept queue** | 已完成握手，等 `accept()` |

**HFT：** 行情 **UDP 组播** 无握手；**TCP 订单通道** 要调 `somaxconn`、`tcp_max_syn_backlog`，监控 **`ss -lnt`** 的 Send-Q/Recv-Q。

---


---

← [本章导读](../README.md)
