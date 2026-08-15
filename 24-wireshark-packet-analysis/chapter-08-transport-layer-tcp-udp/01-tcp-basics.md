# 8.1 传输控制协议（TCP）基础

> 本章：[chapter-summary.md](./chapter-summary.md) · 全书：[../README.md](../README.md) · 网络层：[第7章](../chapter-07-network-layer-proto/chapter-summary.md)

**核心主旨**：TCP 面向连接、可靠交付；报头字段与端口机制是读包的根基。

## 核心知识点

### 核心概念

| 特性 | 说明 |
|------|------|
| **面向连接** | 传数据前须建立连接，全程维护状态（序号、窗口等） |
| **可靠** | 排序、确认、重传、流控 → HTTP 等应用默认依赖 TCP |
| 封装 | IP `protocol == 6` |

### 8.1.1 TCP 报头（抓包必看字段）

| 字段 | 作用 |
|------|------|
| **Source / Destination Port** | 16 位端口，定位应用 |
| **Sequence Number** | 本段数据在字节流中的位置 |
| **Acknowledgment Number** | 期望对方下一字节序号（ACK 有效时） |
| **Flags** | URG、**ACK**、PSH、**RST**、**SYN**、**FIN** |
| **Window Size** | 接收窗口，**流量控制** |
| **Checksum** | 头+数据+伪首部 |
| **Urgent Pointer** | URG=1 时紧急数据偏移 |

**Wireshark**：展开 **Transmission Control Protocol**；可勾选 `Preferences` → TCP → **Relative sequence numbers** 便于读握手后序号。

### 8.1.2 TCP 端口

| 范围 | 用途 |
|------|------|
| **1–1023** | 熟知端口（HTTP 80、HTTPS 443、SSH 22…） |
| **1024–65535** | 客户端常用**临时（ephemeral）**源端口，OS 随机选取 |

| 机制 | 说明 |
|------|------|
| 四元组 | `src IP, src port, dst IP, dst port` 标识一条 TCP 连接 |
| 反向流 | 应答包 **源/目的端口对调** |
| 名称解析 | Wireshark 可能把 `80` 显示为 `http` → 排障可在 `Preferences` → **Name Resolution** 关传输层解析，看**数字端口** |

**过滤器**：`tcp.port == 443` · `tcp.srcport == 54321`

## 抓包/实操记录

| 练习 | 操作 |
|------|------|
| 认字段 | 任选 HTTP 包，对照 Seq/Ack/Win/Flags |
| 临时端口 | 浏览器访问网站，看客户端 `tcp.srcport` 高位 |
| 关闭服务名 | 关传输层解析，确认真实 `dstport` |

## 疑问与总结

- TCP 提供**字节流**，非消息边界；应用层自己划界（HTTP Content-Length 等）。
- 连接建立/关闭见 [§8.1.3–8.1.5](./02-tcp-connection-management.md)；重传/窗口见 [§03](./03-tcp-reliability-flow-control.md) 与第 11 章。
