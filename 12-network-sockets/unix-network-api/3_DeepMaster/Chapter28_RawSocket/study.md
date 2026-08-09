# 第 28 章：原始套接字（厚版）

> [Ch 25](../Chapter25_SignalDriveIO/study.md) · **Ch 28** · [Ch 29](../Chapter29_DataLinkAccess/study.md)  
> 逐节：`28.x_*.md`

> **说明**：上传资料截至第 8 章；第 28 章按 UNP 第 3 版体系整理，请与全本对照验证。

## 本章目标

掌握 **SOCK_RAW** 创建与 root 权限、**IP_HDRINCL** 收发语义、内核过滤规则、**ping/traceroute** 实现思路、**icmpd** 与 Ch 29 的分工。

---

## 小节索引

| 节 | 目录 | 主题 |
|----|------|------|
| 28.1 | [28.1_Overview](./28.1_Overview.md) | 应用场景 |
| 28.2 | [28.2_RawSocket_Create](./28.2_RawSocket_Create.md) | SOCK_RAW、IP_HDRINCL |
| 28.3 | [28.3_RawSocket_Send_Data](./28.3_RawSocket_Send_Data.md) | 输出、字节序 |
| 28.4 | [28.4_RawSocket_Recv_Data](./28.4_RawSocket_Recv_Data.md) | 输入、v4/v6 差异 |
| 28.5 | [28.5_Ping_Program_Implement](./28.5_Ping_Program_Implement.md) | ping |
| 28.6 | [28.6_Traceroute_Program_Implement](./28.6_Traceroute_Program_Implement.md) | TTL + ICMP |
| 28.7 | [28.7_ICMP_Message_Daemon](./28.7_ICMP_Message_Daemon.md) | icmpd |
| 28.8 | [28.8_Summary](./28.8_Summary.md) | 全章收束 |

---

## 一章速记

```text
socket(AF_INET, SOCK_RAW, IPPROTO_ICMP) — 须 root
默认发：载荷=IP 负载，内核写 IP 头；IP_HDRINCL：自带头（移植注意序）
收：不识协议/多数ICMP/全IGMP；不匹配 TCP/UDP
匹配：protocol + bind 目的IP + connect 源IP（AND）
v4 recv 含 IP 头；v6 recv 不含 IP 头
ping：Echo 8/0，id=pid；traceroute：UDP+TTL+ICMP raw
嗅探 TCP/UDP → Ch29 BPF
```

---

## 与前面章节挂钩

| 章节 | 关联 |
|------|------|
| Ch 3 | 字节序 |
| Ch 7 | IP_TTL、IP_HDRINCL |
| Ch 8.9 | UDP 与 ICMP 异步错误 |
| Ch 15 | icmpd Unix 域 IPC |
| Ch 29 | 链路层截获 |

---

## 3_DeepMaster 进度（部分）

| 章 | 状态 |
|----|------|
| 17、20–22、24–25、**28** | 厚版完成 |
| 18、23、29 | 待笔记 |
