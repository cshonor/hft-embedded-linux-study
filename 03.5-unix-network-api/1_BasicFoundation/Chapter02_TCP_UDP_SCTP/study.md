# 第 2 章：传输层 — TCP、UDP 和 SCTP

> 阶段一 [1_BasicFoundation](../) · 逐节 `2.x_*.md` · 上一章：[Ch 1](../Chapter01_Introduction/study.md)

## 本章目标

掌握 **UDP / TCP / SCTP** 定位与机制、**TCP 状态机**（含 TIME_WAIT）、**四元组分路**、**MTU/MSS/缓冲**，为后续套接字编程奠基。

---

## 小节索引

| 节 | 目录 | 主题 |
|----|------|------|
| 2.1 | [2.1_Overview](./2.1_Overview.md) | 三协议定位 |
| 2.2 | [2.2_GeneralGraph](./2.2_GeneralGraph.md) | 协议栈总图 |
| 2.3 | [2.3_UDP_Protocol](./2.3_UDP_Protocol.md) | UDP 数据报 |
| 2.4 | [2.4_TCP_Protocol](./2.4_TCP_Protocol.md) | TCP 可靠字节流 |
| 2.5 | [2.5_SCTP_Protocol](./2.5_SCTP_Protocol.md) | SCTP 消息/多宿/多流 |
| 2.6 | [2.6_TCP_Connect_Terminate](./2.6_TCP_Connect_Terminate.md) | 三次握手、四次挥手、状态图 |
| 2.7 | [2.7_TIME_WAIT_State](./2.7_TIME_WAIT_State.md) | 2MSL、SO_REUSEADDR |
| 2.8 | [2.8_SCTP_Connect_Terminate](./2.8_SCTP_Connect_Terminate.md) | 四路握手、防 SYN 泛洪 |
| 2.9 | [2.9_PortNumber](./2.9_PortNumber.md) | 端口分类、套接字对 |
| 2.10 | [2.10_TCP_Port_ConcurrentServer](./2.10_TCP_Port_ConcurrentServer.md) | 四元组分路 |
| 2.11 | [2.11_Buffer_Size_Limit](./2.11_Buffer_Size_Limit.md) | MTU、MSS、TCP/UDP 输出 |
| 2.12 | [2.12_StandardInternetService](./2.12_StandardInternetService.md) | inetd 标准服务 |
| 2.13 | [2.13_AppProtocolUsage](./2.13_AppProtocolUsage.md) | 常见应用选协议 |
| 2.14 | [2.14_Summary](./2.14_Summary.md) | 小结 |

---

## 一章速记

```text
UDP：无连接不可靠，有边界无流控；要快自管 ACK。
TCP：可靠全双工字节流；ACK/RTT/序/窗；无边界须循环读。
握手 SYN→SYN+ACK→ACK；挥手 FIN→ACK→FIN→ACK；11 状态 netstat。
TIME_WAIT 主动关、2MSL：重传末 ACK + 旧分节消亡；bind 冲突用 REUSEADDR。
SCTP：四路 cookie 握手防 SYN 洪；多宿多流；无半关闭无 TIME_WAIT。
四元组分路已连接套接字；MSS≈1460；TCP 写缓冲等 ACK，UDP write 即返。
```

| 易混 | 一句 |
|------|------|
| TCP 可靠 | 尽最大努力 + 失败通知应用，非「必达对端」 |
| 目的端口分路 | 必须 **四元组**，不能只看 :21 |
| UDP vs TCP 写 | UDP 无真发送缓冲；TCP 等 ACK 才释放 |
