## 10.7–10.8 实验与调优

### 微基准与故障模拟

| 工具 | 用途 |
|------|------|
| **`iperf3`** | TCP/UDP 最大吞吐 |
| **`netperf`** | RPC 风格 RTT |
| **`tc netem`** | 注入 **delay/loss/reorder** — 混沌测试 |

```bash
iperf3 -c server -t 30 -P 4
tc qdisc add dev eth0 root netem delay 2ms loss 0.1%
```

**HFT：** 共置 baseline **iperf + 应用级 ping 订单通道**；netem 在 **测试环境** 验证策略 robustness。

### 系统级 sysctl（Netflix 示例思路）

| 参数 | 方向 | 说明 |
|------|------|------|
| **`net.core.netdev_max_backlog`** | ↑ | 入口队列 |
| **`net.core.somaxconn`** | ↑ | accept 队列 |
| **`net.ipv4.tcp_max_syn_backlog`** | ↑ | SYN 队列 |
| **`net.ipv4.tcp_rmem` / `tcp_wmem`** | ↑ | TCP 窗口上下限 |
| **`net.ipv4.tcp_congestion_control`** | `bbr` | 拥塞算法 |
| **`net.ipv4.tcp_tw_reuse`** | 1 | TIME_WAIT 重用（理解风险） |
| **`net.core.rmem_max` / `wmem_max`** | ↑ | socket buffer 上限 |

**HFT 注意：**

- 共置 **低延迟** 与云 **高吞吐** 参数集 **不同** — 勿盲抄 Netflix 全表。
- 与 **12-HFT ch05/ch06** 合并成 **单一 sysctl runbook**，变更可回滚。

→ [12-HFT ch06](../../../15-HFT-Low-Latency-Practice/chapter-06-低延迟网络/)

### 套接字选项（应用层）

| 选项 | 效果 | HFT |
|------|------|-----|
| **`TCP_NODELAY`** | 禁 Nagle — **小包立即发** | 发单/低延迟 tick 常见 |
| **`TCP_CORK`** | 聚合小包 — 提吞吐 | 批量非紧急数据 |
| **`SO_REUSEPORT`** | 多进程 bind 同一端口 | 收包扩展 |
| **`SO_BUSY_POLL`** |  socket  busy poll | 降 latency、增 CPU |
| **非阻塞 + epoll** | 事件驱动 | Ch 5 · UNP |

→ [09-UNP](../../../09-UNP-Vol1/) · [01-CSAPP Ch11](../../../01-CSAPP-3rd/chapter-11-network-programming/)

---


---

← [本章导读](../README.md)
