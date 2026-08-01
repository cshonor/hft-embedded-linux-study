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

---
