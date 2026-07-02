# 6. UDP、DNS 与其他

### `udpconnect`

追踪 **UDP** “连接”/首次 sendto 目标。

```bash
sudo udpconnect-bpfcc
```

**HFT：** 组播/UDP 行情 — 与 [12-DPDK 组播笔记](../12-DPDK-Low-Latency-Network/01-Intro-Book/notes/chapter-05-组播行情接入.md) 对照。

### `gethostlatency`

追踪 **`getaddrinfo` 等 DNS 解析** 延迟。

```bash
sudo gethostlatency-bpfcc
```

**一针见血：** 慢在 **DNS** 还是 **网络 RTT** — 共置机偶发 `connect` 卡顿常见根因。

### `superping`

内核路径 **ICMP echo** 延迟 — 减少用户态 `ping` 的调度 jitter。

### `ipecn`

追踪 IPv4 **ECN (Explicit Congestion Notification)** 入站事件 — 拥塞信号是否到达。

---
