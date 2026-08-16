## 10.5 分析方法论

### USE 方法（Network Interface）

对 **每个 NIC**（及 bond/VLAN 逻辑口）：

| 字母 | 问什么 | 信号 |
|------|--------|------|
| **U** Utilization | 吞吐 / 协商带宽 | `sar -n DEV`、`nicstat` |
| **S** Saturation | 队列满、重传、overrun | `ip -s link` drop、`netstat -s` retrans、`ss` 队列 |
| **E** Errors | CRC、frame error、drop | `ethtool -S`、`ip -s link` |

→ [附录 A](../../appendix-A-USE方法Linux.md)

### 工作负载与 TCP 分析

| 指标 | 意义 |
|------|------|
| **rx/tx bytes & pps** | 负载量级 |
| **并发连接数** | `ss -s` |
| **Retransmit rate** | 拥塞/丢包 — **`tcpretrans`** |
| **Out-of-order** | 路径多径/重排 |
| **Listen backlog 满** | 丢 SYN |
| **TIME_WAIT 数量** | 短连接扩展性 |

**HFT 红线：**

- 行情路径：**UDP 丢包率、pps、softirq CPU**
- 发单路径：**TCP RTT、重传、Send-Q 积压**

### 抓包（Packet Sniffing）

| 优点 | 代价 |
|------|------|
| 最全协议细节 | **CPU + 磁盘** 开销巨大 |

**Gregg：** 生产 **最后手段** — 优先 `ss`、`nstat`、BPF；抓包限时长、限 filter、mirror 口离线分析。

---


### 常见陷阱

1. USE 只查吞吐——Errors（CRC/frame/drop）和 Saturation（重传/backlog）才是 HFT 关键
2. 抓包当首选——抓包 CPU/磁盘开销巨大，Gregg 说生产最后手段，先 ss/nstat/BPF
3. 重传率不监控——TCP 重传 = 丢包/拥塞，HFT 发单通道重传 = 延迟暴增

<details>
<summary>自测题（点击展开）</summary>

1. 网络 USE 方法中 HFT 最关注什么？
   <details><summary>答</summary>Saturation（重传/backlog 满）和 Errors（CRC/drop）——吞吐够高但重传会导致延迟暴增</details>
2. 为什么抓包是最后手段？
   <details><summary>答</summary>CPU + 磁盘开销巨大——先 ss/nstat/BPF，抓包限时长限 filter</details>
3. HFT 发单通道的关键指标？
   <details><summary>答</summary>TCP RTT、重传率、Send-Q 积压——重传 = 丢包 = 延迟翻倍</details>

</details>


---

← [本章导读](../README.md)
