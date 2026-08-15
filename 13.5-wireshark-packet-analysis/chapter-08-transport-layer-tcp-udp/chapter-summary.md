# 第8章 传输层协议

> 全书：[../README.md](../README.md) · 上一章：[第7章 网络层](../chapter-07-network-layer-proto/chapter-summary.md)

## 整体框架

```text
8.1 TCP：报头 · 端口 · 三次握手 · 四次挥手 · RST
        ↓（重传/窗口 → 第11章）
8.2 UDP：无连接 · 8 字节头 · DNS/DHCP/实时业务
```

| 文件 | 内容 |
|------|------|
| [01-tcp-basics.md](./01-tcp-basics.md) | 8.1 概念、报头、端口 |
| [02-tcp-connection-management.md](./02-tcp-connection-management.md) | 握手、断开、RST |
| [03-tcp-reliability-flow-control.md](./03-tcp-reliability-flow-control.md) | 重传/窗口 Wireshark 入口 → 第11章 |
| [04-udp-protocol.md](./04-udp-protocol.md) | 8.2 UDP |

## 重点难点

| 点 | 过滤器 / 特征 |
|----|----------------|
| 握手第一步 | `tcp.flags.syn==1 && tcp.flags.ack==0` |
| 挥手 | `tcp.flags.fin==1` |
| 拒绝连接 | `tcp.flags.reset==1` |
| 端口显示 | 关闭传输层名称解析看数字 |
| 一条流 | `tcp.stream eq N` |
| UDP | 无 SYN；`udp.port==53` |

## 实操要点

1. 访问 HTTPS 站：标出 3 个握手包 + 首个应用数据包 Seq/Ack。
2. 访问未开放端口：抓 **RST**。
3. `nslookup` 对比 UDP DNS 与 TCP HTTP 连接差异。
4. 卡顿问题跳 [Expert §5.8](../chapter-05-advanced-feature/08-expert-info.md) 与 [第11章](./03-tcp-reliability-flow-control.md)。

## 小节索引

- [8.1 TCP 基础](./01-tcp-basics.md)
- [8.1.3–8.1.5 连接管理](./02-tcp-connection-management.md)
- [TCP 可靠性与流控（延伸）](./03-tcp-reliability-flow-control.md)
- [8.2 UDP](./04-udp-protocol.md)
