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

→ [14-HFT ch06](../../../14-hft-engineering/chapter-06-low-latency-network-protocol/README.md)

### 套接字选项（应用层）

| 选项 | 效果 | HFT |
|------|------|-----|
| **`TCP_NODELAY`** | 禁 Nagle — **小包立即发** | 发单/低延迟 tick 常见 |
| **`TCP_CORK`** | 聚合小包 — 提吞吐 | 批量非紧急数据 |
| **`SO_REUSEPORT`** | 多进程 bind 同一端口 | 收包扩展 |
| **`SO_BUSY_POLL`** |  socket  busy poll | 降 latency、增 CPU |
| **非阻塞 + epoll** | 事件驱动 | Ch 5 · UNP |

→ [12-UNP](../../../03.5-unix-network-api/) · [02-CSAPP Ch11](../../../02-computer-systems/chapter-11-network-programming/)

---


### 常见陷阱

1. 盲抄 Netflix sysctl——Netflix 面向高吞吐 Web，HFT 面向低延迟，参数集完全不同
2. tcp_tw_reuse 不理解就开——TIME_WAIT 重用有风险（旧连接残留数据），需理解场景
3. iperf3 当业务 benchmark——iperf3 测的是 TCP 吞吐上限，不是应用层 tick-to-trade 延迟

<details>
<summary>自测题（点击展开）</summary>

1. 为什么不能直接抄 Netflix 的 sysctl 配置？
   <details><summary>答</summary>Netflix 面向高吞吐 Web（大 buffer/BBR），HFT 面向低延迟（小 buffer/NODELAY）——参数集相反</details>
2. tcp_tw_reuse 的风险是什么？
   <details><summary>答</summary>TIME_WAIT 状态的端口重用——旧连接的残留数据可能被新连接误收，需确认场景适用</details>
3. iperf3 能替代 HFT 应用层 benchmark 吗？
   <details><summary>答</summary>不能——iperf3 测 TCP 吞吐上限，不包含编解码/策略/发单，需应用级 ping/订单通道测试</details>

</details>


---

← [本章导读](../README.md)
