# 10.13 互联网中的 UDP

> 章级精读：[../study.md](../study.md)

## 本节核心目标

归纳 UDP 在现代互联网中的主战场。

---

## 典型应用

| 领域 | 例子 |
|------|------|
| 请求-应答 | **DNS**、SNMP、NTP |
| 实时媒体 | RTP/WebRTC、游戏同步 |
| 隧道 | VXLAN、WireGuard、Teredo |
| 新一代传输 | **QUIC**（UDP 上重建可靠+加密） |
| 组播 | 行情、本地发现（+ [ch09](../../chapter09-broadcast-multicast/study.md)） |

---

## 选型

- 要**可靠/拥塞友好** → TCP 或 QUIC，不要裸 UDP 硬扛公网。
