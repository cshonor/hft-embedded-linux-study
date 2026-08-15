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

→ [Ch 15 BPF](../../chapter-15-bpf/) · [附录 C](../../appendix-C-bpftrace单行命令.md) · [17-BPF](../../../16-bpf-observability/)

### 抓包

| 工具 | 场景 |
|------|------|
| **`tcpdump`** | 服务器 CLI 过滤抓包 |
| **Wireshark** | 离线 decode、TCP 流分析 |

---


### 常见陷阱

1. ss 不加 -tiepm——只看连接列表不看 RTT/重传/cwnd/mss，丢失关键 TCP 内部状态
2. ethtool -S 不看——驱动级统计（NIC drop/no buffer）比 ss 更底层更早发现问题
3. BPF tcpretrans 不用——重传事件+栈定位丢包根因，比 netstat -s 的计数更精确

<details>
<summary>自测题（点击展开）</summary>

1. ss -tiepm 比普通 ss 多看什么？
   <details><summary>答</summary>TCP 内部状态：RTT、cwnd、retrans、mss、mem、BBR 信息——诊断 TCP 性能必需</details>
2. ethtool -S 能发现什么 ss 看不到的？
   <details><summary>答</summary>驱动级统计——NIC drop/no buffer/rx_missed 等，比 ss 更早发现硬件层丢包</details>
3. tcpretrans 比 netstat -s retrans 有什么优势？
   <details><summary>答</summary>tcpretrans 给出每次重传的事件+栈——定位是哪个连接/哪段代码触发重传</details>

</details>


---

← [本章导读](../README.md)
