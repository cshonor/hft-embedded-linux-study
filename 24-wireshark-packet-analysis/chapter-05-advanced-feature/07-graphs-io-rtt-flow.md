# 5.7 图形展示

> 本章：[chapter-summary.md](./chapter-summary.md) · 全书：[../README.md](../README.md)

**核心主旨**：IO 图看吞吐随时间变化；RTT 图看延迟尖峰；Flow Graph 看多主机交互时序。

## 核心知识点

### 5.7.1 IO 图（IO Graphs）

`Statistics` → `IO Graphs`

| 项 | 说明 |
|----|------|
| 用途 | 吞吐量、包速率**随时间**曲线 |
| 多曲线 | 每条曲线可绑不同**显示过滤器** + 颜色/线型 |
| 场景 | 对比「本机 IP」vs「CDN IP」带宽；找吞吐峰值与断崖 |

**操作要点**：添加 Graph → 填 Display filter（如 `ip.addr==x`）→ 选 packets/s 或 bytes/s → 叠加对比。

### 5.7.2 双向时间图（RTT）

| 定义 | **RTT** = 数据到对端 + **确认返回**所需时间（往返） |
| 入口 | `Statistics` → `TCP Stream Graphs` → **Round Trip Time** |

| 读图 | 说明 |
|------|------|
| Y 轴尖峰 | 该时段 RTT 异常高 → 延迟瓶颈 |
| **单向性** | 图对**选定 TCP 流方向**；错方向会误判 |

**易错**：用 **Switch Direction** 切换 A→B / B→A 再对比。

> 与 [第11章 TCP 排障](../chapter-11-network-slow-fix/chapter-summary.md)（若已笔记）联动看重传时段的 RTT 抬升。

### 5.7.3 数据流图（Flow Graph）

`Statistics` → `Flow Graph`（或 Flow Graph 相关菜单，版本略有差异）

| 元素 | 含义 |
|------|------|
| 竖线 | 各主机 |
| 箭头 | 包方向与时间先后 |
| 场景 | DNS 递归、TCP 握手序列、多跳 RPC 的**时序总览** |

## 抓包/实操记录

| 练习 | 目标 |
|------|------|
| IO Graph | 访问大文件下载 → 看 bytes/s 峰值时段 |
| RTT | 对卡顿 TCP 流开 RTT graph → 标出尖峰对应包号回查 |
| Flow Graph | 单次 DNS 解析 → 看 client → resolver → upstream 箭头链 |

## 疑问与总结

- IO Graph 统计受 **捕获时长、丢包** 影响；镜像过载时曲线可能「假低」。
- RTT 基于捕获包推算，**丢包**时 RTT 点可能缺失或异常。
