# 17.4 总结

> 章级精读：[../study.md#ch17-exam](../study.md#ch17-exam)

## 本节核心目标

明确保活定位与**应用层心跳**分工。

---

## 定位

- **非** RFC 强制；多数系统**默认关闭**或间隔很长
- 服务器用来清理**僵尸客户端**、辅助发现半开

---

## TCP 保活 vs 应用心跳

| | TCP 保活 | 应用 Ping/Pong |
|--|----------|----------------|
| 验证 | **内核**存活 | **业务**健康 |
| 典型用途 | 喂 NAT/防火墙 映射 | IM/MQTT/WebSocket SLA |

**双管齐下**才是高可用长连接标准。

---

## Go / Rust 实战

| 语言 | 要点 |
|------|------|
| **Go** | 默认开 Keepalive；`KeepAlivePeriod` 官方约 **15s**（非系统 2h） |
| **Rust** | `socket.set_keepalive(Some(Duration::from_secs(30)))` |
| **通用** | 勿只靠 OS 2h；NAT 数分钟即失效 → **应用层心跳**必做 |

---

## 下一章

- [ch18 安全](../../chapter18-network-security/study.md)
