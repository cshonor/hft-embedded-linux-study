# 8.2 用户数据报协议（UDP）

> 本章：[chapter-summary.md](./chapter-summary.md) · 全书：[../README.md](../README.md)

**核心主旨**：UDP 无连接、「尽力而为」、小报头——DNS/DHCP/实时业务的首选传输层。

## 核心知识点

### 与 TCP 对比

| 项 | TCP | UDP |
|----|-----|-----|
| 连接 | 需握手/挥手 | **无**连接状态 |
| 可靠/顺序 | 协议保证 | **不保证**；应用自管 |
| 报头 | 20+ 字节起 | **8 字节**固定 |
| 典型应用 | HTTP、SSH | **DNS、DHCP**、QUIC(UDP)、RTP |

### 无连接（Connectionless）

- 发前不握手，发后不辞别；每个 **UDP 数据报** 独立路由。
- 丢包、乱序由**应用层**决定是否重试（DNS 超时重查、RTP 跳帧等）。

### UDP 报头

| 字段 | 说明 |
|------|------|
| Source Port | 源端口（可为 0，如部分 DNS 客户端） |
| Destination Port | 目的端口（DNS **53**） |
| Length | 头 + 数据总长度 |
| Checksum | 头+数据+伪首部；IPv4 下可为 0（历史）；IPv6 常强制 |

**Wireshark**：`udp` · `udp.port == 53` · `udp.length`

### 抓包表征

| 现象 | 说明 |
|------|------|
| 请求-响应 | 两个独立 UDP 包，**无** SYN/FIN |
| 端口 | 同 TCP，反向流交换 src/dst port |
| 多路复用 | 单主机多 DNS 查询靠 **源端口** 区分 |

> **拓展**：**RTP/RTSP** 视频、在线游戏 — 小包、高频、可容忍丢包；过滤器 `rtp` · `udp.port` 范围；与 TCP 下载对比 IO Graph _burst 形态。

## 抓包/实操记录

| 实验 | 步骤 |
|------|------|
| DNS | `nslookup` → `udp.port==53` → Follow UDP Stream |
| 对比 TCP | 同站 HTTPS：`tcp` 有握手；DNS 无 |
| DHCP | 局域网抓 `bootp` / `dhcp`（UDP 67/68） |

```bash
tshark -r cap.pcapng -Y "dns" -T fields -e udp.srcport -e udp.dstport -e dns.qry.name
```

## 疑问与总结

- UDP 「快」因无状态机与重传；**QUIC** 在 UDP 上实现可靠（HTTP/3），别与裸 UDP 混淆。
- 校验和错误时 Expert 可能标 **Bad checksum**（offload 可能导致假阳性，见网卡校验卸载设置）。
