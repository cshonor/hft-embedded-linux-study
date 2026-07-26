# 第 22 章：高级 UDP 套接字编程（厚版）

> [Ch 8](../../1_BasicFoundation/Chapter08_BasicUDPSocket/study.md) · [Ch 21](../Chapter21_Multicast/study.md) · **Ch 22**  
> 逐节：`22.x_*.md`

> **说明**：上传资料截至第 8 章；第 22 章按 UNP 第 3 版体系整理，请与全本对照验证。

## 本章目标

掌握 UDP **目的地址/接口**、**MSG_TRUNC**、UDP/TCP 选型、应用层**可靠请求-应答**、多 bind、并发服困境、IPv6 **pktinfo** 与 **PMTU**。

---

## 小节索引

| 节 | 目录 | 主题 |
|----|------|------|
| 22.1 | [22.1_Overview](./22.1_Overview.md) | 章概览 |
| 22.2 | [22.2_Flag_DestIP_InterfaceIndex](./22.2_Flag_DestIP_InterfaceIndex.md) | recvmsg 辅助数据 |
| 22.3 | [22.3_Datagram_Truncation](./22.3_Datagram_Truncation.md) | MSG_TRUNC |
| 22.4 | [22.4_UDP_TCP_Scene_Choice](./22.4_UDP_TCP_Scene_Choice.md) | 选型法则 |
| 22.5 | [22.5_UDP_Reliable_Transform](./22.5_UDP_Reliable_Transform.md) | RTO、序号 |
| 22.6 | [22.6_Bind_Specified_Interface](./22.6_Bind_Specified_Interface.md) | 具体 bind 优先 |
| 22.7 | [22.7_Concurrent_UDP_Server](./22.7_Concurrent_UDP_Server.md) | 并发困境 |
| 22.8 | [22.8_IPv6_Packet_Info](./22.8_IPv6_Packet_Info.md) | in6_pktinfo |
| 22.9 | [22.9_IPv6_Path_MTU_Control](./22.9_IPv6_Path_MTU_Control.md) | PMTU 选项 |
| 22.10 | [22.10_Summary](./22.10_Summary.md) | 全章收束 |

---

## 一章速记

```text
多宿主收：recvmsg + IP_RECVDSTADDR/RECVIF 或 IPV6_RECVPKTINFO
截断：recvmsg 查 MSG_TRUNC
必 UDP：广播/多播；大块可靠→用 TCP
请求-应答：动态 RTO + 序号 + 退避
并发 UDP：迭代/线程池为主；fork 新端口慎用
IPv6：源地址/接口 override；PMTU 与 USE_MIN_MTU
```

---

## 与前面章节挂钩

| 章节 | 关联 |
|------|------|
| Ch 8 | 基础 UDP、connect、丢包 |
| Ch 14 | recvmsg、辅助数据、超时 |
| Ch 20–21 | 广播/多播必 UDP |
| Ch 7 | SO_REUSEADDR、多 bind |
| Ch 26 | 线程池并发 UDP |

---

## 3_DeepMaster 进度（部分）

| 章 | 状态 |
|----|------|
| 17 ioctl | 厚版完成 |
| 20 广播 | 厚版完成 |
| 21 多播 | 厚版完成 |
| **22 高级 UDP** | **厚版完成** |
| 18、23–25、28–29 | 待笔记 |
