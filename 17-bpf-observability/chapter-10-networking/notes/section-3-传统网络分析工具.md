# 3. 传统网络分析工具

| 工具 | 用途 |
|------|------|
| **`ss -tinap`** | **首选** socket 统计 — 状态、重传、RTT、cwnd 等 |
| `ip -s link` | 网卡级计数、drop |
| `nstat` | 内核 SNMP 计数器 |
| `netstat` | 旧式，优先 `ss` |
| `sar -n DEV` | 接口吞吐 |
| **`nicstat`** | 网卡利用率、吞吐 |
| **`ethtool -S`** | 驱动/NIC 硬件计数（drop、fifo…） |
| **`tcpdump`** | 抓包 — **见线路，不见 PID/内核栈** |

```bash
ss -tinap
ip -s link show eth0
ethtool -S eth0 | grep -i drop
```

**抓包盲区（Gregg 强调）：**

| tcpdump 能 | tcpdump 不能 |
|------------|--------------|
| 线路上的包 | **哪个 PID** 发送 |
| 五元组 | **内核为何重传**（拥塞 vs 本地 drop） |
| pcap 文件 | **调用栈**、socket 缓冲满 |

→ 这正是 BPF 工具的价值。


### 常见陷阱

1. **只依赖 netstat/ss 做网络分析** — netstat/ss 只显示连接状态和计数器，看不到包级延迟和重传原因；需配合 BPF 工具做深度分析
2. **忽视 ifconfig 统计的局限性** — ifconfig 的 RX/TX errors/dropped 是聚合计数器，不知道何时、为什么丢包；BPF 能追踪丢包时刻和路径
3. **用 ping 测延迟忽略内核栈开销** — ping 测的是 RTT（含内核网络栈处理），不是纯网络传输延迟；HFT 需用 BPF 分段测量各层耗时

<details>
<summary>📝 自测题（点击展开）</summary>

1. **传统网络工具有哪些？各自能看什么？**

   <details>
   <summary>参考答案</summary>

   (1) ifconfig/ip -s link：接口级 RX/TX 包数/字节/错误/丢弃；(2) netstat/ss：连接状态表（ESTABLISHED/TIME_WAIT 等）、socket 统计；(3) ping/traceroute：RTT 和路径；(4) tcpdump/wireshark：包级抓取和分析；(5) ethtool：网卡统计、ring buffer、offload。局限：聚合计数器看不到事件级细节和延迟分布。

   </details>

2. **传统工具相比 BPF 的网络分析盲区有哪些？**

   <details>
   <summary>参考答案</summary>

   (1) 重传原因：netstat 只给重传计数→tcpretrans 追踪每次重传的时间和序列号；(2) 连接延迟：netstat 无→tcpconnlat 测每次 connect 的耗时；(3) 内核栈路径耗时：tcpdump 只看包→BPF 追踪 tcp_sendmsg→dev_queue_xmit 各层耗时；(4) 丢包位置：ifconfig 只给计数→BPF 追踪丢包发生在哪层。

   </details>

3. **HFT 网络延迟分析为什么不能用 ping？**

   <details>
   <summary>参考答案</summary>

   Ping 测的是 ICMP RTT：(1) ICMP 走不同内核路径（不走 TCP 栈）；(2) RTT 包含两端内核处理+网络传输，无法分段定位；(3) 粒度太粗（毫秒级），HFT 需要微秒级。替代方案：用 BPF 在 tcp_sendmsg 和 tcp_recvmsg 上打时间戳，分段测量应用→内核→网卡→网络→对端→返回各段耗时。

   </details>

</details>

---
