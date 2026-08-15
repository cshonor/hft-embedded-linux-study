# 8.1 延伸：可靠传输、重传与流量控制

> 本章：[chapter-summary.md](./chapter-summary.md) · 全书：[../README.md](../README.md) · 详解：[第11章 让网络不再卡](../chapter-11-network-slow-fix/chapter-summary.md) · Expert：[§5.8](../chapter-05-advanced-feature/08-expert-info.md)

**说明**：教材在 TCP 基础后于第 11 章展开**重传、滑动窗口、拥塞控制**；本节给出 Wireshark 读包入口，避免与握手/断开重复。

## 核心知识点

### 可靠性在包里的表象

| 机制 | 抓包可见 |
|------|----------|
| 确认 | **ACK** 标志 + Ack number 递增 |
| 重传 | 相同 Seq 再次出现；Expert：**Retransmission** / **Fast Retransmission** |
| 乱序 | **Out-of-order**；常伴 Duplicate ACK |
| 流控 | **Window Size** 变小；**Zero Window** |
| 拥塞 | 超时重传、窗口剧烈变化（第 11 章细讲） |

### 常用显示过滤器

| 场景 | 过滤器 |
|------|--------|
| 重传 | `tcp.analysis.retransmission` |
| 快速重传 | `tcp.analysis.fast_retransmission` |
| 重复 ACK | `tcp.analysis.duplicate_ack` |
| 零窗口 | `tcp.analysis.zero_window` |
| 乱序 | `tcp.analysis.out_of_order` |

### TCP Stream Graphs（GUI）

`Statistics` → `TCP Stream Graphs`：**Time-Sequence (Stevens)**、**Window Scaling**、**Round Trip Time** — 与第 5、11 章配合。

## 抓包/实操记录

| 练习 | 操作 |
|------|------|
| Expert | `Analyze` → `Expert Information` → 筛 Warning |
| 单连接 | `tcp.stream eq N` 隔离一条流 |
| RTT 尖峰 | TCP Stream Graph → Round Trip Time |

## 疑问与总结

- 抓包点**丢包**会出现 `Previous segment not captured`，不一定是网络真丢。
- 完整因果链（Dup ACK → Fast Retransmit → Recovery）在第 11 章笔记中展开。
