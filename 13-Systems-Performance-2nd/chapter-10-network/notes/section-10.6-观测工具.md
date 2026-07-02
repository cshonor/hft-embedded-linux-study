## 10.6 观测工具

### 传统统计

| 工具 | 用途 | 关键 |
|------|------|------|
| **`ss -tiepm`** | 套接字 **TCP 内部状态** | RTT、cwnd、retrans、mem、BBR 信息 |
| **`ip -s link`** | 接口吞吐、drop、overrun | `RX/TX errors` |
| **`nstat` / `netstat -s`** | SNMP 协议栈计数 | retrans、failed connects |
| **`sar -n DEV`** | 历史接口吞吐 | 容量/事后 |
| **`nicstat`** | 接口 %util 类指标 | 忙不忙 |
| **`ethtool -S`** | **驱动级** 统计 | NIC drop、no buffer |

```bash
ss -tiepm | head -50          # 看 RTT、重传、mss
ip -s link show eth0
nstat -az | grep -i retrans
ethtool -S eth0 | grep -i drop
```

### BPF / BCC

| 工具 | 作用 |
|------|------|
| **`tcplife`** | 每连接生命周期、吞吐 — **极实用** |
| **`tcptop`** | 按进程网络吞吐 |
| **`tcpretrans`** | 重传事件 + 栈 |
| **`tcpconnect` / `tcpaccept`** | 连接建立追踪 |
| **`bpftrace`** | 自定义丢包、内核栈 |

→ [Ch 15 BPF](../../chapter-15-bpf/) · [附录 C](../../appendix-C-bpftrace单行命令.md) · [14-BPF](../../../14-BPF-Performance-Tools/)

### 抓包

| 工具 | 场景 |
|------|------|
| **`tcpdump`** | 服务器 CLI 过滤抓包 |
| **Wireshark** | 离线 decode、TCP 流分析 |

---


---

← [本章导读](../README.md)
