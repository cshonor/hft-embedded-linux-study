## 10.6 观测工具

> 章节导航：[10.5 分析方法论](./section-10.5-分析方法论.md) · 上一篇 ← · 下一篇 [10.7–10.8 实验与调优](./section-10.7-10.8-实验与调优.md) · [本章导读](../README.md)

**本节讲什么**：网络观测的三层工具（套接字统计 / BPF 事件级 / 抓包）、`ss -tiepm` 输出逐字段精读、ethtool 驱动级统计的解读。

### 要点

| # | 要点 | 一句话 |
|---|------|--------|
| 1 | `ss -tiepm` 是 TCP 诊断的**瑞士军刀** | 一条命令看 RTT/cwnd/重传/内存 |
| 2 | 计数级 vs **事件级** | nstat 告诉你有多少，tcpretrans 告诉你**谁和哪** |
| 3 | `ethtool -S` 比 ss **更底层更早** | NIC ring 丢包 ss 看不见 |
| 4 | tcplife 是**事后审计**神器 | 每连接完整生命周期 |
| 5 | 抓包只解**协议谜题** | 字段级问题的最后仲裁 |

---

### 一、传统统计层

| 工具 | 用途 | 关键 |
|------|------|------|
| **`ss -tiepm`** | 套接字 **TCP 内部状态** | RTT、cwnd、retrans、mem（见下方精读） |
| **`ip -s link`** | 接口吞吐、drop、overrun | RX/TX errors、dropped |
| **`nstat` / `netstat -s`** | SNMP 协议栈计数 | retrans、failed connects、RcvbufErrors |
| **`sar -n DEV`** | 历史接口吞吐 | 容量/事后 |
| **`nicstat`** | 接口 %util 类指标 | 忙不忙 |
| **`ethtool -S`** | **驱动级**统计 | NIC drop、no buffer、rx_missed |

**⭐ `ss -tiepm` 输出逐字段精读**：

```
ESTAB  0  32768  10.1.2.3:gateway  10.4.5.6:5001  ...（-t 已显示的状态行）
  cubic wscale:7,7 rto:204 rtt:0.023/0.005 ato:40 mss:1448 cwnd:10
  bytes_acked:1.2M bytes_received:890.5K segs_out:1200 segs_in:980
  send 45.2Mbps lastsnd:120 lastrcv:8 lastack:8 pacing_rate:90.4Mbps
  retrans 0/12 rcv_rtt:1 rcv_space:65535 notsend:0
  skmem:(r0,rb131072,t0,tb46080,f0,w0,o0,bl0,d0)
```

| 字段 | 含义 | HFT 判读 |
|------|------|---------|
| `rtt:0.023/0.005` | RTT 平滑值/偏差（µs 或 ms 视量级） | 共置应在 **<100µs**；rtt 波动大 = 路径抖动 |
| `cwnd:10` | 拥塞窗口（报文段数） | 被打小说明有丢包历史 |
| `rto:204` | 重传超时（ms） | 一次 RTO 重传 = 这个量级的停顿 |
| `mss:1448` | 最大段大小 | 40B TCP 头 + 12B 选项 |
| `retrans 0/12` | **重传 0 / 总发 12 段**（或已重传/总重传，版本语义注意） | 非零即红线（[10.5](./section-10.5-分析方法论.md)） |
| `send 45.2Mbps` | 当前发送速率 | — |
| `notsend:0` | 未发送排队字节 | **Send-Q 语义的应用层视角**，非零 = 发不出去 |
| `skmem:(r0,rb131072,t0,tb46080,...)` | socket 内存（读队列/读缓冲上限/写...） | rb 打满 → 应用读慢 → UDP 会丢 |
| `lastsnd/lastrcv` | 距上次收发 ms | 心跳判断 |

**用法纪律**：`ss -tiepm` 不加过滤会打出全部连接——生产上先 `ss -tn state established '( dport = :5001 or sport = :5001 )'` 缩小范围。

**ethtool -S 精读**（驱动级，名字因驱动而异）：

```bash
ethtool -S eth0 | grep -iE 'drop|miss|no_buf|discard'
# rx_missed:      NIC RX ring 满（软中断跟不上）——[10.5 UDP 丢包①]
# rx_no_buffer / rx_no_dma_resources: 缓冲分配失败
# tx_discards:    发送侧丢
```

**/proc/net/softnet_stats**（软中断队列）：

```bash
awk '{print $2}' /proc/net/softnet_stats   # 第2列=dropped，非零 = backlog 溢出 [10.5 ②]
```

### 二、BPF / BCC 层（事件级）

| 工具 | 作用 | 独特价值 |
|------|------|---------|
| **`tcplife`** | 每连接生命周期（起止/时长/流量） | **事后审计**——哪个连接何时建立了多久 |
| **`tcptop`** | 按进程网络吞吐排行 | 归属 |
| **`tcpretrans`** | 重传**事件** + 内核栈 | 定位哪个连接哪段代码触发重传 |
| **`tcpconnect` / `tcpaccept`** | active/passive open 逐次 + 耗时 | 连接建立延迟（含 TCP 握手 RTT） |
| `bpftrace` 单行 | 自定义丢包/内核栈 | 见 [ch15](../../chapter-15-bpf/notes/section-15.2-bpftrace.md) |

**计数级 vs 事件级的本质区别**：`nstat` 的 retrans 计数告诉你「有 37 次重传」；`tcpretrans` 告诉你「PID 4321 在 14:03:22 的 connect 路径上重传了发往 10.4.5.6:5001 的段」——**归因能力差一个维度**。生产红线监控用事件级（[ch15 的升格工作流](../../chapter-15-bpf/notes/section-15.1.7-BCC-vs-bpftrace.md)）。

**tcplife 输出**（连接结束时刻输出一行）：

```
PID   COMM       LADDR           LPORT RADDR           RPORT TX_KB RX_KB MS
4321  strategy   10.1.2.3        45678 10.4.5.6        5001     12    45 8900
```

一条连接活了 8.9 秒、发 12KB 收 45KB——会话审计/异常连接筛查（陌生地址、异常时长）的好工具。

### 三、抓包层

| 工具 | 场景 |
|------|------|
| **`tcpdump`** | 服务器 CLI 过滤抓包（**必带 filter + -c + timeout**） |
| **Wireshark** | 离线 decode、TCP 流分析（IO graph、专家信息） |

抓包的合法场景：协议字段谜题（中间盒篡改 MSS/选项）、应用层协议协商失败、重排/重复的包级证据。永远镜像口离线分析优先（[10.5](./section-10.5-分析方法论.md)）。

### 四、工具选型速查

| 问题 | 第一工具 | 深挖 |
|------|---------|------|
| 这条 TCP 连接健康吗？ | `ss -tiepm` | tcpretrans 事件 |
| 接口丢包吗？ | `ip -s link` | ethtool -S（驱动级） |
| UDP 丢在哪一段？ | softnet_stats + ethtool -S | bpftrace 定点 |
| 谁在收发流量？ | tcptop / ss | tcplife 审计 |
| 协议字段对不对？ | tcpdump 短窗口 | Wireshark 离线 |
| 历史趋势？ | sar -n DEV | 监控系统时序库 |

### HFT / 嵌入式关联

- **巡检常驻**：`ss -tiepm`（发单连接快照）+ softnet_stats + ethtool -S 关键 drop 计数——低开销，进 cron/监控。
- **事件触发**：tcpretrans（重传定罪）+ tcpconnect（建连延迟）。
- **嵌入式**：小内存设备上 tcpdump 不可承受——bpftrace 单行 + 计数文件是全部观测面。

### 衔接

- 上一节：[10.5 分析方法论](./section-10.5-分析方法论.md)
- 下一节：[10.7–10.8 实验与调优](./section-10.7-10.8-实验与调优.md)
- 关联：[ch15 BPF](../../chapter-15-bpf/)、[附录 C](../../appendix-C-bpftrace单行命令.md)、[12-UNP](../../../03.5-unix-network-api/)（socket 机制）、[06.7-BPF](../../../06.7-bpf-observability/)

---

### 常见陷阱

1. **ss 不加 -tiepm**——只看连接列表，丢掉 RTT/重传/cwnd/mss 全部 TCP 内部状态。
2. **ethtool -S 不看**——NIC ring 丢包（rx_missed）比 ss 层更早发生，ss 根本看不见。
3. **重传只看计数不看事件**——netstat -s 的数字无法归因；tcpretrans 才能定位到连接和栈。
4. **ss 全量输出**——生产机器连接多，先按端口/状态过滤再精读。

<details>
<summary>自测题（点击展开）</summary>

1. ss -tiepm 比普通 ss 多看什么？
   <details><summary>答</summary>TCP 内部状态：rtt/偏差、cwnd、rto、mss、retrans、发送速率、notsend、skmem——诊断 TCP 性能必需。</details>
2. ethtool -S 能发现什么 ss 看不到的？
   <details><summary>答</summary>驱动级统计——rx_missed（NIC ring 满）、rx_no_buffer 等硬件层丢包，发生在内核统计之前。</details>
3. tcpretrans 比 netstat -s retrans 有什么优势？
   <details><summary>答</summary>事件级：每次重传的连接四元组+内核栈——直接归因到连接和触发路径；计数级只知道总数。</details>
4. ss 的 notsend 字段什么含义？
   <details><summary>答</summary>应用已写但尚未发送的排队字节（Send-Q 的语义化视角）——非零持续说明网络或对端窗口堵塞。</details>
5. softnet_stats 的 dropped 列对应哪段丢包？
   <details><summary>答</summary>netdev backlog 溢出——软中断处理跟不上，包在内核入口队列被丢（NIC ring 之后、socket 之前）。</details>

</details>


---

← [本章导读](../README.md)
