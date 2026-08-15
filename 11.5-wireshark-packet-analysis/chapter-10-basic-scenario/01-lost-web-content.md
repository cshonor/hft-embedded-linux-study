# 10.1 丢失的网页内容

> 本章：[chapter-summary.md](./chapter-summary.md) · 全书：[../README.md](../README.md) · DNS：[§9.2](../chapter-09-application-layer-proto/02-dns-protocol.md)

**核心主旨**：网页部分内容缺失时，用 **DNS 次数 vs IP 会话数** 发现「未查 DNS 却连旧 IP」的黑洞连接。

## 核心知识点

### 故障现象

- 浏览 ESPN 类站点：**极慢**，图片/资源大量缺失。

### 分析过程

| 步 | 工具/动作 | 发现 |
|----|-----------|------|
| 1 | `Statistics` → **HTTP** → Requests；**Protocol Hierarchy** | **7** 次 DNS 查询/响应 ↔ **7** 次 HTTP 请求（宏观一致） |
| 2 | `Statistics` → **Conversations**（IPv4） | **8** 个 IP 会话 ≠ 7 → **多出的第 8 个** |
| 3 | 筛第 8 会话 | 客户端 → **未知 IP** 发 **SYN**，**无 SYN/ACK** |
| 4 | Expert / 过滤 | `tcp.analysis.retransmission` 持续 **~95s** |

```text
7 DNS + 7 HTTP  但  8 个 TCP 会话  →  有连接未经过本次 DNS
```

### 因果与结论

| 根因 | 说明 |
|------|------|
| **本地 DNS 缓存** | 仍缓存**已失效**的内容节点 IP |
| 行为 | 部分 URL **跳过 DNS**，直连**不可用旧 IP** |
| 表现 | SYN 黑洞 + 重传 → 页面「缺块」、整体变慢 |

| 处理 | `ipconfig /flushdns`（Windows）或等待 TTL 过期 |

> **拓展**：`dns.flags.response == 0` 对比时间线；无对应查询的 `tcp.stream` 即可疑缓存直连。

## 抓包/实操记录

| 过滤器 | 用途 |
|--------|------|
| `http` | HTTP 请求数 |
| `dns` | DNS 查询数 |
| `tcp.flags.syn==1 && tcp.flags.ack==0` | 未完成的连接尝试 |
| `tcp.analysis.retransmission` | 黑洞重传 |

**练习**：对慢页抓包 → Conversations 排序 → 找 **无 DNS 前置** 的 SYN 流。

## 疑问与总结

- 不是「DNS 坏了」，而是 **DNS 次数对但少查了一次关键名**。
- CDN/多子域场景更易出现「部分子域缓存过期」。
