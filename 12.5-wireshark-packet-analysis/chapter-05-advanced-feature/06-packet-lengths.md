# 5.6 数据包长度

> 本章：[chapter-summary.md](./chapter-summary.md) · 全书：[../README.md](../README.md)

**核心主旨**：用包长分布鸟瞰捕获内容——大数据传输 vs 纯控制信令。

## 核心知识点

### 功能入口

`Statistics` → `Packet Lengths`

| 展示 | 说明 |
|------|------|
| 直方图 / 区间表 | 各长度区间的包数占比 |

### 推断法则

| 长度区间（教材典型） | 常见含义 |
|----------------------|----------|
| **1280–2559 字节** | 携带载荷的**数据传输**（HTTP 下载、FTP、大段 TLS 应用数据） |
| **40–79 字节** | 多为**无载荷或极小载荷**的 TCP 控制包 |

**原因**：以太网 + IP + TCP 头部约 **40–54+ 字节**；ACK、纯 SYN、RST 等常落在此范围。

| 结论 | 应用 |
|------|------|
| 打开 pcap 先看 Packet Lengths | 建立「主要是 bulk 传输还是控制/ping」的假设，再下钻 Expert / TCP 图 |

## 抓包/实操记录

| 对比 | 预期 |
|------|------|
| 下载大文件 | 长度分布右移，大包占比高 |
| 空闲链路 + keepalive | 小 ACK 包居多 |
| ping  flood | 固定较小 ICMP 包 |

## 疑问与总结

- jumbo frame 环境区间会上移；与路径 **MTU** 有关（见 TCP/IP 笔记）。
- 小包多不一定是故障；需结合 [Protocol Hierarchy](./02-protocol-hierarchy.md) 与业务类型。
