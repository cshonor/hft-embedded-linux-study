# 2. 网络基础知识 (Background)

### Linux 网络栈路径

```
应用程序
  → Socket API（用户态）
  → 传输层 TCP/UDP
  → 网络层 IP
  → qdisc（排队规则）
  → 网卡驱动 / NAPI
  → NIC
```

→ 协议语义：[13-TCP-IP](../../../../11-tcpip-protocols/) · 内核栈：[14-Rosen](../../../../12-kernel-networking/) · Socket API：[12-UNP](../../../../03.5-unix-network-api/) · 实战：[12-PNP](../../../../04-cpp/M5-cpp-network-programming/)

### 内核绕过 (Kernel Bypass)

| 技术 | 说明 |
|------|------|
| **DPDK** | 用户态 PMD 轮询 NIC — **绕过内核栈**，避免 per-packet 复制/ syscall |
| **XDP / tc-BPF** | 仍在内核，但在 **最早** 收包点执行 BPF — 见 [note-XDP与tc-BPF](../../note-XDP与tc-BPF.md) |

**HFT 分工：**

```
热路径行情/下单  →  DPDK / 内核 bypass（14-DPDK）
共置辅助流量     →  内核栈 + 本章 BPF 观测
对照实验         →  同一机：BPF 看内核栈 vs DPDK 看用户态环
```

### TCP 机制（观测相关）

| 机制 | BPF 工具关联 |
|------|--------------|
| **SYN Backlog / Listen Backlog** | `tcpsynbl` — 队列满 → SYN 丢包 |
| **重传**（超时 / 快速重传） | **`tcpretrans`** |
| **发送/接收缓冲区** 动态调整 | `sormem`、`tcpwin` |
| **Nagle** | `tcpnagle` — 小 packet 延迟 |
| **TFO** (TCP Fast Open) | 连接延迟分析时区分 |

### 卸载与分段

| 技术 | 作用 |
|------|------|
| **TSO/GSO** | 大块 TCP 分段 offload |
| **GRO/LRO** | 接收合并 |
| **影响** | `netsize` 可见软件分段前后包大小分布 |

### 常见延迟指标

| 指标 | 含义 |
|------|------|
| DNS 解析 | `gethostlatency` |
| ICMP RTT | `superping`（比用户态 ping 少调度噪声） |
| TCP 连接建立 | `soconnlat`、`tcpconnect` 时间线 |
| 首字节 | **`so1stbyte`** |


### 常见陷阱

1. **混淆 TCP 状态机的观测点** — TCP 有 11 种状态（ESTABLISHED/TIME_WAIT/SYN_SENT 等），不同状态的转换有不同的 tracepoint；不知道状态转换路径就无法选对观测点
2. **忽视网卡 ring buffer 的大小和溢出** — ring buffer 太小在高 PPS 下会丢包；`ethtool -g eth0` 查看，`ethtool -G eth0 rx 4096` 调大；HFT 应确保不因 ring buffer 溢出丢包
3. **混淆 backlog 队列和 accept 队列** — backlog 是 SYN 队列（半连接），accept 队列是已完成握手待 accept 的连接；队列满会导致丢 SYN 或连接拒绝

<details>
<summary>📝 自测题（点击展开）</summary>

1. **TCP 连接建立的关键步骤和观测点是什么？**

   <details>
   <summary>参考答案</summary>

   (1) 客户端 SYN_SENT → 发 SYN（`tracepoint:syscalls:sys_enter_connect`）；(2) 服务端收到 SYN → SYN 队列（`tracepoint:tcp:tcp_v4_syn_recv_sock`）；(3) 三次握手完成 → accept 队列（`tracepoint:syscalls:sys_enter_accept`）；(4) ESTABLISHED。tcpconnlat 测量从 connect 到 ESTABLISHED 的延迟。

   </details>

2. **网卡 ring buffer 对 HFT 网络性能有什么影响？**

   <details>
   <summary>参考答案</summary>

   Ring buffer 是网卡和内核之间的包缓冲区。PPS（包/秒）高时，如果 ring buffer 太小，内核来不及消费就溢出丢包。HFT 场景：小包高频交易每秒可能数千包，默认 ring buffer（256）可能不够。检查：`ethtool -g eth0`；调大：`ethtool -G eth0 rx 4096 tx 4096`。

   </details>

3. **SYN 队列（backlog）和 accept 队列满了会怎样？如何检测？**

   <details>
   <summary>参考答案</summary>

   SYN 队列满：新 SYN 包被丢弃（客户端看到 connect timeout）——`netstat -s | grep \"overflowed\"`。Accept 队列满：已完成握手的连接等待 accept，超时后拒绝——`ss -lnt` 的 Recv-Q 列非零。BPF 追踪：`tracepoint:tcp:tcp_v4_syn_recv_sock /args->status == 0/ { @drop++ }`。

   </details>

</details>

---
