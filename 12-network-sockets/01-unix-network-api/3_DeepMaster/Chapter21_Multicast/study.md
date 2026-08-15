# 第 21 章：多播（厚版）

> [Ch 20 广播](../Chapter20_Broadcast/study.md) · **Ch 21** · [Ch 22](../Chapter22_AdvancedUDPSocket/)（待笔记）  
> 逐节：`21.x_*.md`

> **说明**：上传资料截至第 8 章；第 21 章按 UNP 第 3 版体系整理，请与全本对照验证。

## 本章目标

掌握多播地址与 **MAC 映射/碰撞**、LAN/WAN（IGMP/MLD）、**SSM**、套接字选项与 **mcast_*** 封装、接收端 **SO_REUSEADDR**、SAP/SNTP 实践。

---

## 小节索引

| 节 | 目录 | 主题 |
|----|------|------|
| 21.1 | [21.1_Overview](./21.1_Overview.md) | vs 广播；仅 UDP |
| 21.2 | [21.2_Multicast_Address](./21.2_Multicast_Address.md) | D 类、ff00::、32:1 |
| 21.3 | [21.3_LAN_Multicast_Broadcast](./21.3_LAN_Multicast_Broadcast.md) | 硬件过滤 |
| 21.4 | [21.4_WAN_Multicast_Transfer](./21.4_WAN_Multicast_Transfer.md) | IGMP/MLD、PIM |
| 21.5 | [21.5_Source_Specific_Multicast](./21.5_Source_Specific_Multicast.md) | ASM/SSM |
| 21.6 | [21.6_Multicast_Socket_Option](./21.6_Multicast_Socket_Option.md) | join/TTL/IF/loop |
| 21.7 | [21.7_Mcast_Join_Related_Func](./21.7_Mcast_Join_Related_Func.md) | mcast_join 等 |
| 21.8 | [21.8_Multicast_Dg_Cli](./21.8_Multicast_Dg_Cli.md) | 发送客户 |
| 21.9 | [21.9_Multicast_Session_Declare](./21.9_Multicast_Session_Declare.md) | SAP/SDP |
| 21.10 | [21.10_Send_Receive_Multicast_Data](./21.10_Send_Receive_Multicast_Data.md) | 自环与自过滤 |
| 21.11 | [21.11_SNTP_Protocol_Practice](./21.11_SNTP_Protocol_Practice.md) | 多播校时 |
| 21.12 | [21.12_Summary](./21.12_Summary.md) | 全章收束 |

---

## 一章速记

```text
仅 UDP；Join 才收（网卡+IGMP/MLD）
IPv4 224/4；MAC 01:00:5E 低23位→32个IP共一MAC
收：SO_REUSEADDR→bind→IP_ADD_MEMBERSHIP/mcast_join
发：sendto 多播地址；TTL 默认1；发方常不必 join
SSM：指定源；IGMPv3/MLDv2
```

---

## 与前面章节挂钩

| 章节 | 关联 |
|------|------|
| Ch 20 | 广播对比 |
| Ch 7.5 | SO_REUSEADDR |
| Ch 8 | UDP |
| Ch 11 | mcast_* 与 getaddrinfo 同思路 |
| Ch 17 | 接口列表、MULTICAST 标志 |
| Ch 22 | 高级 UDP 多播选项延伸 |
