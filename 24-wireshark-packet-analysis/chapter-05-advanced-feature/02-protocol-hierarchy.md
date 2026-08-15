# 5.2 基于协议分层结构的统计

> 本章：[chapter-summary.md](./chapter-summary.md) · 全书：[../README.md](../README.md)

**核心主旨**：用协议分层占比快照诊断流量异常、配置错误与网段属性。

## 核心知识点

### 协议分层统计（Protocol Hierarchy）

`Statistics` → `Protocol Hierarchy`

| 输出 | 含义 |
|------|------|
| 树状协议列表 | 各协议占**总包数/总字节**的百分比 |
| 全局快照 | 一眼看 L2→L3→L4→应用 的构成 |

### 基线对比与异常诊断

| 现象 | 推断 |
|------|------|
| ARP 日常 ~10%，当前 **50%** | 异常广播/ARP 风暴、扫描、环路嫌疑 |
| 未部署 STP 却出现 **STP** 流量 | 某交换机误发 BPDU / 配置错误 |
| 突然出现大量 **未知协议** | 非标准端口、隧道、恶意封装 |

### 网段属性推断（经验）

| 协议占比特征 | 可能网段类型 |
|--------------|--------------|
| 大量 ICMP + SNMP | IT 管理/监控网段 |
| 大量 SMTP / IMAP | 邮件/订单处理 |
| 大量 RTP/RTCP、UDP 高位端口 | 语音/视频会议 |
| 大量 SMB、RPC | 文件/域控 |

> **拓展**：为每种业务（DB 同步、视频会议）建立「正常 Hierarchy 百分比」模板，存入 [Configuration Profile](../chapter-03-wireshark-intro/04-get-started.md)。

## 抓包/实操记录

| 练习 | 步骤 |
|------|------|
| 基线快照 | 正常时段抓包 → Hierarchy 截图/抄录 Top5 协议 % |
| 故障对比 | 卡顿时段再抓 → ARP/TCP 重传相关协议是否飙升 |
| 过滤后统计 | 显示 `!arp` 后再开 Hierarchy（或 Limit to display filter）看「去掉 ARP 后」结构 |

## 疑问与总结

- Hierarchy 百分比受 **抓包位置** 影响（镜像口、本机环回不同）。
- 与 [Expert Info](./08-expert-info.md) 结合：占比异常 + 专家告警可交叉验证。
