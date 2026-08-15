# HFT 场景 01：TCP 延迟分析

> [总览](./00-overview.md) · [基础：ch11 网络慢排查](../chapter-11-network-slow-fix/chapter-summary.md) · [TCP 可靠性](../chapter-08-transport-layer-tcp-udp/03-tcp-reliability-flow-control.md)

**核心问题**：HFT 系统中，每次 TCP 重传 = 交易延迟尖峰。一次超时重传可导致 200μs–数 ms 的延迟，足以让套利机会消失。

## 1. 延迟来源与 Wireshark 过滤器

| 延迟类型 | 成因 | Wireshark 过滤器 | 典型延迟影响 |
|----------|------|-----------------|-------------|
| **超时重传** | RTO 内未收到 ACK | `tcp.analysis.retransmission` | 200μs–数 ms（RTO 指数退避） |
| **快速重传** | 3× Dup ACK 触发 | `tcp.analysis.fast_retransmission` | 100μs–1ms |
| **零窗口** | 接收方不读数据 | `tcp.analysis.zero_window` | 数 ms–数十 ms |
| **窗口更新** | 接收方恢复窗口 | `tcp.analysis.window_update` | 间歇性停滞 |
| **乱序** | 包到达顺序错 | `tcp.analysis.ack_rtt` + 序号对比 | 触发 Dup ACK |
| **Nagle + Delayed ACK** | 小包 + 延迟确认 | 看包间隔 + PSH 标志 | 40ms 延迟（默认） |

## 2. RTT 测量

Wireshark 自动计算每个 ACK 的 RTT（`tcp.analysis.ack_rtt` 字段）。

### 图形化 RTT 分析

```
Statistics → TCP Stream Graphs → Round Trip Time Graph
```

| 操作 | 说明 |
|------|------|
| 切换 Y 轴 | 对数坐标更适合 HFT（μs 级延迟） |
| 点击尖峰 | 跳转到对应包，查看上下文 |
| 导出数据 | 右键 → Save As → CSV → Python 分析 |

### tshark 提取 RTT

```bash
# 提取所有 ACK 的 RTT，单位秒
tshark -r trade.pcapng -Y "tcp.analysis.ack_rtt" \
  -T fields -e frame.time_relative -e tcp.analysis.ack_rtt -e tcp.stream

# 统计 RTT 分布（P50/P95/P99）
tshark -r trade.pcapng -Y "tcp.analysis.ack_rtt" \
  -T fields -e tcp.analysis.ack_rtt | \
  awk '{sum+=$1; count++; if($1>max)max=$1; vals[count]=$1} 
  END{asort(vals); printf "count=%d avg=%.0fus p50=%.0fus p95=%.0fus p99=%.0fus max=%.0fus\n", count, sum/count*1e6, vals[int(count*0.5)]*1e6, vals[int(count*0.95)]*1e6, vals[int(count*0.99)]*1e6, max*1e6}'
```

## 3. HFT 延迟分析实战流程

```
步骤 1：抓包
  → tcpdump -nni eth0 -w trade.pcapng -s 0 'tcp port 443 or tcp port 8000'
  → 必须在交易服务器上抓（抓包点影响 RTT 测量）

步骤 2：快速定位延迟尖峰
  → Wireshark: Statistics → TCP Stream Graphs → RTT Graph
  → 找到 RTT > 100μs 的点

步骤 3：分析根因
  → tcp.analysis.retransmission → 丢包（网络拥塞 or 网卡丢包）
  → tcp.analysis.zero_window → 应用处理不过来
  → 包间隔 > 40ms 且无重传 → Nagle/Delayed ACK

步骤 4：关联应用层时间戳
  → tshark -r trade.pcapng -Y "tcp.analysis.retransmission" \
      -T fields -e frame.time_relative -e tcp.seq -e tcp.analysis.rto
  → 与交易日志对齐，确认延迟是否导致 missed fill
```

## 4. Nagle 与 Delayed ACK 陷阱

HFT 系统应**禁用 Nagle**（`TCP_NODELAY`），否则小包会被攒着等 ACK。

| 检查项 | Wireshark 表现 |
|--------|---------------|
| Nagle 开启 | 小包后等待 ACK 才发下一个，间隔 ~40ms |
| Nagle 关闭 | 小包连续发送，无等待 |
| Delayed ACK | ACK 间隔 ~40ms 或等下一个数据包捎带 |

```bash
# 检测 Nagle/Delayed ACK 模式：看数据包间隔分布
tshark -r trade.pcapng -Y "tcp.len > 0" \
  -T fields -e frame.time_delta_displayed | \
  awk '{if($1>0.001) print $1}' | sort -n | uniq -c | sort -rn | head -10
# 如果 0.04 附近有大量聚集 → Nagle/Delayed ACK 问题
```

## 5. 零窗口分析

交易系统如果 Socket 接收缓冲区满了（应用处理慢），会发零窗口。

```bash
# 统计零窗口事件
tshark -r trade.pcapng -Y "tcp.analysis.zero_window" -c 20

# 看零窗口前后的窗口大小变化
tshark -r trade.pcapng -Y "tcp.window_size_value < 1000" \
  -T fields -e frame.time_relative -e tcp.window_size_value -e tcp.srcport
```

| 解决方案 | 说明 |
|----------|------|
| 增大 Socket 缓冲区 | `setsockopt(SO_RCVBUF)` / `setsockopt(SO_SNDBUF)` |
| 调整 `tcp_rmem` / `tcp_wmem` | 内核自动调参上限 |
| 应用层优化 | 减少 read 间隔，批量处理 |
| `TCP_NODELAY` | 避免小包积压 |

## 6. HFT 自测题

1. 交易系统 RTT 从 50μs 升到 500μs，但无重传。可能原因有哪些？如何用 Wireshark 确认？
2. 如何区分「网络丢包导致的重传」和「网卡 RX 队列丢包导致的重传」？
3. 为什么在**接收端**抓包可能看不到重传包本身？
4. 一个 TCP 流的 P99 RTT 是 2ms，但应用层延迟是 5ms。差的 3ms 在哪里？Wireshark 能看到吗？

## 交叉引用

- [基础：TCP 可靠性与流控](../chapter-08-transport-layer-tcp-udp/03-tcp-reliability-flow-control.md)
- [基础：TCP 重传](../chapter-11-network-slow-fix/01-tcp-retransmission.md)
- [基础：TCP 流控](../chapter-11-network-slow-fix/02-tcp-flow-control.md)
- [HFT 场景 02：NIC offload](./02-nic-offload-impact.md)
- [HFT 模块：系统性能](../../14-systems-performance/)
- [HFT 模块：TCP/IP 协议](../../11-tcpip-protocols/)
