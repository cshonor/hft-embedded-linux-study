# 4. 套接字层 (Socket API) 工具

### `sockstat` / `sofamily` / `soprotocol`

| 工具 | 统计 |
|------|------|
| `sockstat` | accept、connect 等 **事件频率** |
| `sofamily` | 地址族 IPv4/IPv6/UNIX… |
| `soprotocol` | TCP/UDP/… 协议分布 |

**用途：** **工作负载表征** — 连接型 vs 数据报、IPv6 占比。

### `soconnect` / `soaccept`

追踪 **connect / accept** — **IP、端口、PID、comm**。

```bash
sudo soconnect-bpfcc
sudo soaccept-bpfcc
```

**HFT：** 策略是否意外 **outbound 建连**（合规 API、DNS、telemetry）。

### `socketio` / `socksize`

按进程统计 socket **读写次数** 与 **字节数直方图**。

```bash
sudo socketio-bpfcc 5
sudo socksize-bpfcc
```

### `sormem`

**接收队列 (receive queue)** 大小直方图 — 缓冲溢出 → 内核 drop。

```bash
sudo sormem-bpfcc
```

### `soconnlat` / `so1stbyte`

| 工具 | 测量 |
|------|------|
| `soconnlat` | **连接建立** 延迟 + 栈 |
| `so1stbyte` | **首字节** 延迟 + 栈 |

**HFT：** 区分「TCP 握手慢」vs「连接后应用层首包慢」— 对 **网关/行情源** 接入排查极有用。


### 常见陷阱

1. **在 HFT 热路径上用 socketsnoop 逐行追踪** — socketsnoop 每次 send/recv 都打印一行，高频交易路径上会产生大量输出和开销；应用 Map 聚合或按进程过滤
2. **混淆 connect 延迟和 send 延迟** — connect 延迟是 TCP 握手时间（网络 RTT），send 延迟是数据从应用到内核的时间（通常微秒级）；两者原因不同
3. **忽视 accept 队列堆积** — 如果服务端 accept 速度跟不上握手完成速度，accept 队列堆积导致延迟；ss -lnt 的 Recv-Q 非零表示堆积

<details>
<summary>📝 自测题（点击展开）</summary>

1. **套接字层的关键 BPF 工具有哪些？**

   <details>
   <summary>参考答案</summary>

   (1) socketsnoop：追踪 connect/accept/send/recv 系统调用（逐事件打印）；(2) tcpconnect：追踪主动连接建立（谁连了谁）；(3) tcpaccept：追踪被动连接接受；(4) tcplife：记录连接生命周期（开始/结束/持续时间/字节数）；(5) tcptop：按连接统计吞吐量。

   </details>

2. **connect 延迟和 send 延迟分别说明什么问题？**

   <details>
   <summary>参考答案</summary>

   connect 延迟（tcpconnlat 测量）：TCP 三次握手耗时——反映网络 RTT 和对端响应速度。延迟大说明网络慢或对端处理慢。send 延迟（socketsnoop 测量）：send 系统调用耗时——反映内核网络栈处理速度和发送队列状态。延迟大说明内核栈慢或发送队列拥塞。HFT 两者都需关注。

   </details>

3. **HFT 场景中如何安全地使用套接字层 BPF 工具？**

   <details>
   <summary>参考答案</summary>

   (1) 按进程过滤：`-p $(pidof myapp)` 只追踪目标进程，避免全系统开销；(2) 短跑：套接字层工具在每次 send/recv 触发，高频交易每秒数千次，短跑 5-10 秒足够采样；(3) 用 Map 聚合替代逐行：`tracepoint:syscalls:sys_enter_sendto { @[comm] = count() }` 看频率分布而非逐条；(4) 用 tcplife 看连接概览而非逐包追踪。

   </details>

</details>

---
