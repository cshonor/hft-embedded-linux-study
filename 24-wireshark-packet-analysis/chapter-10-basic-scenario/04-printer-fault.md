# 10.4 打印机故障

> 本章：[chapter-summary.md](./chapter-summary.md) · 全书：[../README.md](../README.md) · TCP 窗口：[§8.1 延伸](../chapter-08-transport-layer-tcp-udp/03-tcp-reliability-flow-control.md)

**核心主旨**：大批量打印卡死——用 **重传 + 零窗口** 区分网络拥塞与打印机内存故障。

## 核心知识点

### 故障现象

- 大作业打印数页后**卡死**；纯局域网 **TCP** 打印象流量。

### 分析过程

| 阶段 | 抓包表现 |
|------|----------|
| 初期 | 三次握手正常；**1460 字节**段 + ACK 正常 |
| 中期 | **[TCP Retransmission]** 发送方未收到 ACK |
| 窗口 | 打印机方向 **Window Size 递减** → **Zero Window** |
| Expert | `tcp.analysis.zero_window` · `tcp.analysis.retransmission` |

### 因果与结论

| 判定 | 说明 |
|------|------|
| **非**典型拥塞 | 局域网、无大量交叉流时，先怀疑**接收端** |
| **是**设备问题 | 打印机 **内存/固件** 无法继续收缓冲 → 停 ACK → 零窗口 |

| 处理 | 减 spool 复杂度、固件升级、硬件检测 |

> **拓展**：**IO Graph** 看在 Zero Window 前吞吐是否骤降。

## 抓包/实操记录

| 过滤器 | 用途 |
|--------|------|
| `tcp.port == 9100` | 常见 RAW 打印（视型号） |
| `ip.addr == <打印机IP>` | 隔离流 |
| `tcp.analysis.zero_window` | 零窗口事件 |
| `tcp.stream eq N` | 单作业 |

**TCP Stream Graph** → Window Scaling：观察接收窗口塌陷时间点。

## 疑问与总结

- 服务器也可能零窗口（慢消费者）；此处特征是**打印机为接收方**且作业特定。
- 与 [§10.3.3](./03-no-internet-access.md) SYN 黑洞不同：此处连接已建立且有数据阶段。
