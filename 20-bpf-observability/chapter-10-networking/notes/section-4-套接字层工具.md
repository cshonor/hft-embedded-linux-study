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

---
