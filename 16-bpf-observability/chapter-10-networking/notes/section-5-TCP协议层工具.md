# 5. TCP 协议层工具

### `tcpconnect` / `tcpaccept`

在 **TCP 栈更深处** 挂载（比 socket 层更贴近协议状态）。

```bash
sudo tcpconnect-bpfcc
sudo tcpaccept-bpfcc
```

→ [Ch 3 BCC 清单](../../chapter-03-performance-analysis/) 含 `tcpconnect`。

### `tcplife` — 会话总结 🔴

连接 **建立时记录**，**关闭时一行总结**：

- 本地/远程 IP:端口  
- 收发总字节  
- **会话持续时间 (Lifespan)**  

```bash
sudo tcplife-bpfcc
```

| 优点 | 说明 |
|------|------|
| **低开销** | 不需抓包 |
| **HFT** | 看清某行情 TCP 会话活了多久、传了多少 — 异常长连/短连 |

### `tcptop`

TCP 版 **top** — 按 **发送/接收 Kbytes** 排序进程。

```bash
sudo tcptop-bpfcc
```

### `tcpretrans` — 重传追踪 🔴

追踪 **TCP 重传** — 地址、TCP 状态。

```bash
sudo tcpretrans-bpfcc
```

| 解读 | 含义 |
|------|------|
| 重传突增 | 拥塞、丢包、对端问题、**本机网卡 drop** |
| 与延迟尖刺同相 | 网络层首要嫌疑 |

**HFT runbook 三件套之一：** `runqlat` + `profile` + **`tcpretrans`**（Ch 3）。

### `tcpsynbl`

**SYN 积压队列** 直方图 — 警告 **SYN 丢包**（队列溢出）。

```bash
sudo tcpsynbl-bpfcc
```

**场景：** 接入层 accept 跟不上 SYN flood 或 legit 连接风暴。

### `tcpwin` / `tcpnagle`

| 工具 | 作用 |
|------|------|
| `tcpwin` | **拥塞窗口 cwnd** 变化 — 可导出 CSV 画拥塞控制 |
| `tcpnagle` | **Nagle 算法** 导致的发送延迟 |

**HFT：** 低延迟 socket 通常 **`TCP_NODELAY`** — `tcpnagle` 验证是否误开 Nagle。


### 常见陷阱

1. **把重传等同于网络拥塞** — 重传可能由丢包（网络问题）或内核栈延迟（调度问题）引起；tcpretrans 能看重传时刻和序列号，但不直接告诉原因
2. **忽视 TCP 重传对 HFT 的微秒级影响** — 一次重传至少增加一个 RTT 的延迟（通常几十微秒到毫秒）；HFT 策略循环中一次重传可能导致超时
3. **混淆 tcpretrans 和 tcpretrans 的追踪范围** — tcpretrans 只追踪内核 TCP 栈的重传；DPDK 用户态 TCP 栈的重传不在此工具范围内

<details>
<summary>📝 自测题（点击展开）</summary>

1. **TCP 协议层的关键 BPF 工具有哪些？**

   <details>
   <summary>参考答案</summary>

   (1) tcpretrans：追踪 TCP 重传事件（时间、源/目的、序列号、原因）；(2) tcpconnlat：测量 TCP 连接建立延迟；(3) tcptop：按连接统计发送/接收字节数；(4) tcprtt：TCP RTT 分布直方图；(5) tcpsize：读写大小分布。这些工具基于 tcp_tracepoint 或 kprobe 实现。

   </details>

2. **TCP 重传对 HFT 延迟有什么影响？如何追踪？**

   <details>
   <summary>参考答案</summary>

   一次重传至少增加一个 RTO（重传超时，通常 200ms+）或快重传时间（3 个重复 ACK，约 1 RTT）。HFT 策略循环中一次重传可能导致策略超时。追踪：BCC `tcpretrans`——显示每次重传的时间、连接、序列号、状态。bpftrace：`tracepoint:tcp:tcp_retransmit_skb { @[src,dst] = count() }` 按连接统计重传次数。

   </details>

3. **tcprtt 直方图对 HFT 有什么价值？**

   <details>
   <summary>参考答案</summary>

   tcprtt 显示 TCP RTT（往返时间）的分布直方图——内核根据 ACK 返回时间动态计算的 RTT 估计值。HFT 价值：(1) RTT 基线——正常 RTT 是多少，异常时偏移多少；(2) RTT 分布尾部——P99 RTT 是否远超中位数（说明有偶发网络抖动）；(3) 按连接对比——不同对端交易所的 RTT 差异。`bpftrace -e 'kprobe:tcp_rtt_estimator { @rtt = hist(arg2 / 1000) }'`。

   </details>

</details>

---
