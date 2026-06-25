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

---
