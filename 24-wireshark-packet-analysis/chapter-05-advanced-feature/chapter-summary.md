# 第5章 Wireshark 高级特性

> 全书：[../README.md](../README.md) · 上一章：[第4章 玩转捕获数据包](../chapter-04-capture-packet/chapter-summary.md)

## 整体框架

```text
5.1 Endpoints / Conversations（Top Talkers）
        ↓
5.2 Protocol Hierarchy（与基线比 %）
        ↓
5.3 名称解析（性能 · 恶意流量禁外连 DNS）
        ↓
5.4 Dissector · Decode As
        ↓
5.5 Follow Stream（TCP/UDP/SSL/HTTP）
        ↓
5.6 Packet Lengths（大包传数据 · 小包多控制）
        ↓
5.7 IO Graph · RTT · Flow Graph
        ↓
5.8 Expert Info（TCP 排障核心）
```

| 小节 | 主题 | 文件 |
|------|------|------|
| 5.1 | 端点与会话 | [01-endpoints-conversations.md](./01-endpoints-conversations.md) |
| 5.2 | 协议分层统计 | [02-protocol-hierarchy.md](./02-protocol-hierarchy.md) |
| 5.3 | 名称解析 | [03-name-resolution.md](./03-name-resolution.md) |
| 5.4 | 协议解析 / Decode As | [04-protocol-dissectors-decode-as.md](./04-protocol-dissectors-decode-as.md) |
| 5.5 | 流跟踪 | [05-follow-stream.md](./05-follow-stream.md) |
| 5.6 | 数据包长度 | [06-packet-lengths.md](./06-packet-lengths.md) |
| 5.7 | 图形展示 | [07-graphs-io-rtt-flow.md](./07-graphs-io-rtt-flow.md) |
| 5.8 | 专家信息 | [08-expert-info.md](./08-expert-info.md) |

## 重点难点

| 点 | 说明 |
|----|------|
| **Top Talkers** | Endpoints/Conversations 按 Bytes 排序 + WHOIS |
| **Hierarchy vs 基线** | ARP/STP 占比突变 → 配置或风暴 |
| **DNS 解析** | 大文件卡顿；恶意 pcap **禁外部 resolver** |
| **Decode As Save** | 勿污染长期配置 |
| **Follow Stream + TLS** | 需密钥才解密 |
| **RTT 图方向** | Switch Direction |
| **Expert Warning** | 重传、零窗口、未捕获段 |

## 实操要点

1. 每个 baseline pcap 存一份 **Protocol Hierarchy** 截图或百分比。
2. 内网维护无后缀 **hosts**；分析恶意样本断外网。
3. 卡顿问题：`Expert Info` → `TCP Stream Graph RTT` → `IO Graph` 三联。
4. 过滤器见 [cheatsheet/notes.md](../cheatsheet/notes.md)。

## 小节索引

- [5.1 端点和网络会话](./01-endpoints-conversations.md)
- [5.2 基于协议分层结构的统计](./02-protocol-hierarchy.md)
- [5.3 名称解析](./03-name-resolution.md)
- [5.4 协议解析](./04-protocol-dissectors-decode-as.md)
- [5.5 流跟踪](./05-follow-stream.md)
- [5.6 数据包长度](./06-packet-lengths.md)
- [5.7 图形展示](./07-graphs-io-rtt-flow.md)
- [5.8 专家信息](./08-expert-info.md)
