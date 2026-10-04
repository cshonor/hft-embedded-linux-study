## 10.5 分析方法论

> 章节导航：[本章导读](../README.md) · 下一篇 [10.6 观测工具](./section-10.6-观测工具.md)

**本节讲什么**：网络接口的 USE 检查、TCP 健康的指标集（重传/乱序/backlog/TIME_WAIT）、UDP 与 TCP 两条路径的不同分析框架、抓包的定位（最后手段）。

### 要点

| # | 要点 | 一句话 |
|---|------|--------|
| 1 | USE 只查吞吐是常见失误 | **重传/drop/backlog** 才是延迟杀手 |
| 2 | UDP 与 TCP 的分析框架**完全不同** | 无连接 vs 有连接 |
| 3 | 重传 = 延迟暴增的**确定性前兆** | HFT 发单通道红线指标 |
| 4 | softirq CPU 是**收包路径的税** | 高 pps 时 ksoftirqd 抢核 |
| 5 | 抓包是**最后手段** | 先 ss/nstat/BPF |

---

### 一、USE 方法（Network Interface）

对**每个 NIC**（及 bond/VLAN 逻辑口）：

| 字母 | 问什么 | 信号 | 工具 |
|------|--------|------|------|
| **U** Utilization | 吞吐 / 协商带宽 | rx/tx bytes、pps | `sar -n DEV`、`nicstat` |
| **S** Saturation | 队列满、重传、overrun | drop、retrans、backlog 溢出 | `ip -s link`、`netstat -s`、`ss -lnt` |
| **E** Errors | CRC、frame error、drop | 驱动级错误 | `ethtool -S`、`ip -s link` |

**注意「逻辑口」**：bond 主备、VLAN 子接口、网桥端口各有独立统计——USE 要对每一层都过一遍（物理口正常不代表 bond 口正常）。

**与磁盘的差异**：磁盘的 U 看忙时比；网络的 U 看**带宽占比 + pps 占比两个维度**——小包场景带宽 5% 但 pps 已顶格（64B 小包线速 = 14.88Mpps，软中断先跪）。

> 完整检查表：[附录 A](../../appendix-A-USE方法Linux.md)

### 二、TCP 路径分析（发单通道）

| 指标 | 意义 | 哪里看 |
|------|------|--------|
| **RTT** | 网络 RTT 实测 | `ss -ti` 的 rtt 字段 |
| **Retransmit rate** | 拥塞/丢包——**延迟暴增前兆** | `tcpretrans`（事件级）、netstat -s（计数级） |
| **cwnd / ssthresh** | 拥塞窗口状态 | `ss -ti` |
| **Out-of-order** | 路径多径/重排 | netstat -s |
| **Listen backlog 满** | 丢 SYN（accept 跟不上） | `ss -lnt`（Recv-Q 接近 Send-Q 上限） |
| **Send-Q 积压** | 发不出去（对端窗口/网络拥塞） | `ss -tn` 的 Send-Q |
| **TIME_WAIT 数量** | 短连接扩展性 | `ss -s` 汇总 |

**重传为什么是红线**：一次重传 = RTO 等待（最短 200ms 级）或 fast retrans（一个 RTT 延迟）——对 µs 级 SLA 的发单通道，**一次重传就把这单的延迟打到 ms 级**。重传率的监控要在事件级（tcpretrans 带栈）而非计数级。

**Send-Q 积压的归因链**：

```
Send-Q 持续有积压
  ├─ 对端 rcv 窗口小（对端处理慢/应用读得慢）→ ss -ti 看 peer 窗口
  ├─ cwnd 被打小（丢包历史）→ ss -ti 看 cwnd + 重传计数
  └─ 本地 qdisc 排队（发不出去）→ tc -s qdisc show
```

### 三、UDP 路径分析（行情通道）

UDP 无连接——TCP 的窗口/重传分析全不适用，框架换成**丢包定位**：

```
丢包可能在哪？
  ① NIC RX ring 满（softirq 处理跟不上）   → ethtool -S 的 rx_missed/rx_no_buffer
  ② netdev backlog 满（软中断队列溢出）     → /proc/net/softnet_stats 的 dropped 列
  ③ socket 接收缓冲满（应用读得慢）         → ss -u -m 的接收队列 / netstat -s 的 RcvbufErrors
  ④ 应用层丢（epoll 循环慢）               → 应用计数（收包序号空洞检测）
  └ ⑤ 网络中丢（中间设备）                  → 发端计数 vs 收端计数对比
```

**行情数据的空洞检测**：feed 序号连续性检查是最直接的应用层证据——序号跳变 = 中间丢包，结合 ①-④ 的内核计数定位丢在哪一段。

**softirq CPU 的监控**：高 pps 时 `ksoftirqd/N` 占用是收包路径的直接成本——监控 `/proc/softirqs` 的 NET_RX 增速与 `mpstat` 的 %soft；热路径机器要给收包中断分配 dedicated 核（[ch6 中断亲和](../../chapter-06-cpus/)）。

### 四、工作负载特征化

| 问题 | 工具 |
|------|------|
| 负载量级（bytes/pps）？ | `sar -n DEV 1` |
| 并发连接数与状态分布？ | `ss -s` |
| 哪个进程在收发？ | `tcptop`/`iftop`（[10.6](./section-10.6-观测工具.md)） |
| 连接生命周期？ | `tcplife` |
| RTT 与重传？ | `ss -tiepm` + `tcpretrans` |

### 五、抓包的定位

| 优点 | 代价 |
|------|------|
| 最全协议细节（字段级） | **CPU + 磁盘**开销巨大；敏感数据合规风险 |

**Gregg**：生产**最后手段**——优先 `ss`、`nstat`、BPF（计数/事件已覆盖 90% 问题）；抓包只在协议层谜题（字段错、中间盒篡改）时短窗口使用：

```bash
# 必须带 filter + 限包数 + 限时长
timeout 10 tcpdump -i eth0 -c 10000 -w /tmp/net.pcap 'udp port 5001 and host 10.1.2.3'
# 更好：mirror 口/交换机端口镜像 → 离线机器分析
```

### 六、HFT 双路径红线汇总

| 路径 | 红线指标 | 阈值思路 |
|------|---------|---------|
| 行情（UDP 组播） | 丢包率（①-⑤ 分段计数）、pps、softirq CPU | 丢包 = 数据不完整 → 补救流程；softirq 持续 >50% 单核 → 加队列/分流 |
| 发单（TCP） | RTT、重传、Send-Q 积压 | 任何重传 = 该单延迟 ms 级；Send-Q 非零持续 = 网络或对端问题 |

### 衔接

- 下一节：[10.6 观测工具](./section-10.6-观测工具.md)（ss -tiepm 精读）
- 关联：[ch2 USE](../../chapter-02-methodologies/)、[ch6 softirq/中断亲和](../../chapter-06-cpus/)、[14-HFT ch06 低延迟网络协议](../../../14-hft-engineering/chapter-06-low-latency-network-protocol/README.md)、[12-UNP](../../../03.5-unix-network-api/)（socket 层机制）

---

### 常见陷阱

1. **USE 只查吞吐**——Errors（CRC/frame/drop）和 Saturation（重传/backlog）才是 HFT 关键。
2. **抓包当首选**——CPU/磁盘开销巨大，先 ss/nstat/BPF。
3. **重传率不监控**——TCP 重传 = 丢包 = 延迟暴增；要事件级（tcpretrans）不是计数级。
4. **UDP 丢包只查网络**——①②③④ 四段本地丢包（NIC ring/backlog/socket buf/应用）比网络丢更常见。
5. **小包场景只看带宽**——pps 才是上限维度，softirq 先于带宽饱和。

<details>
<summary>自测题（点击展开）</summary>

1. 网络 USE 方法中 HFT 最关注什么？
   <details><summary>答</summary>Saturation（重传/backlog 满）和 Errors（CRC/drop）——吞吐够高但重传会导致延迟暴增。</details>
2. UDP 丢包的本地四段排查？
   <details><summary>答</summary>①NIC RX ring 满（ethtool -S rx_missed）②netdev backlog 溢出（softnet_stats dropped）③socket 缓冲满（RcvbufErrors）④应用 epoll 读慢（序号空洞检测）——再加⑤网络段（两端计数对比）。</details>
3. 为什么重传是发单通道的红线？
   <details><summary>答</summary>一次重传至少一个 RTT 的额外延迟（fast retrans）或 200ms 级 RTO——µs 级 SLA 直接破防；事件级监控（tcpretrans 带栈）才能定位根因。</details>
4. Send-Q 积压的三个可能原因？
   <details><summary>答</summary>对端接收窗口小（对端慢）、cwnd 被历史丢包打小、本地 qdisc 排队——ss -ti 看窗口/cwnd，tc -s qdisc 看本地队列。</details>
5. 为什么抓包是最后手段？
   <details><summary>答</summary>CPU + 磁盘开销巨大——先 ss/nstat/BPF（计数/事件级已覆盖 90%）；抓包限时长限 filter，或用 mirror 口离线分析。</details>

</details>


---

← [本章导读](../README.md)
