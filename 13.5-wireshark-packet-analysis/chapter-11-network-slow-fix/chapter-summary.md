# 第11章 让网络不再卡

> 全书：[../README.md](../README.md) · 上一章：[第10章 实战场景](../chapter-10-basic-scenario/chapter-summary.md)

## 整体框架

```text
11.1 重传 · Dup ACK · 快速重传 · SACK
        ↓
11.2 接收窗口 · 零窗口 · Keep-Alive
        ↓
11.3 抓包点选址（发送端/接收端）
        ↓
11.4 三段时间：线路 / 客户端 / 服务端
        ↓
11.5 站点 / 主机 / 应用基线
```

| 小节 | 文件 |
|------|------|
| 11.1 | [01-tcp-retransmission.md](./01-tcp-retransmission.md) |
| 11.2 | [02-tcp-flow-control.md](./02-tcp-flow-control.md) |
| 11.3 | [03-tcp-troubleshooting-placement.md](./03-tcp-troubleshooting-placement.md) |
| 11.4 | [04-latency-locate-framework.md](./04-latency-locate-framework.md) |
| 11.5 | [05-network-baseline.md](./05-network-baseline.md) |

## 11.6 小结

修复卡顿不能靠盲目重启，应围绕：

1. **错误恢复**：`tcp.analysis.retransmission`、`duplicate_ack`、`fast_retransmission`
2. **流量控制**：`tcp.analysis.zero_window` → 主机性能
3. **延迟三法则**：SYN 段 / GET 前 / 响应首包 三段 **Delta**
4. **基线**：正常时 Hierarchy、IO Graph、依赖 IP

将「网络卡」变成**毫秒级节点**证据。

## 重点难点

| 点 | 要点 |
|----|------|
| RTO 退避 | 超时重传越来越慢 |
| 3 Dup ACK | 触发快速重传 |
| 抓包位置 | 重传@发送端，Dup ACK@接收端 |
| 零窗口 | 接收方背压 |
| <0.1s 基准 | HTTP 完整首包往返（环境相关） |
| 高峰本机抓包 | 可能加重故障 |

## 实操要点

1. 慢连接：`tcp.stream eq N` + Expert + TCP Stream Graphs。
2. 建三份 baseline（站点/关键主机/核心应用）。
3. 过滤器见 [cheatsheet](../cheatsheet/notes.md)。

## 小节索引

- [11.1 TCP 错误恢复](./01-tcp-retransmission.md)
- [11.2 TCP 流控制](./02-tcp-flow-control.md)
- [11.3 排障选址](./03-tcp-troubleshooting-placement.md)
- [11.4 延迟定位框架](./04-latency-locate-framework.md)
- [11.5 网络基线](./05-network-baseline.md)
