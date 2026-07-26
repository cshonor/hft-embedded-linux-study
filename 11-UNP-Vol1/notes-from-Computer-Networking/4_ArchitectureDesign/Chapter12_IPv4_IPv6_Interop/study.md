# 第 12 章：IPv4 与 IPv6 的互操作性（厚版）

> [Ch 11](../../2_AdvancedSkill/Chapter11_Name_Address_Convert/study.md) · **Ch 12**（`4_ArchitectureDesign`）· [Ch 13](../../2_AdvancedSkill/Chapter13_Daemon_Inetd/)（待笔记）  
> 逐节：`12.x_*.md`

> **说明**：上传资料截至第 8 章；第 12 章框架来自目录，细节按 UNP 第 3 版整理，请与全本对照验证。

## 本章目标

理解 **双栈主机**上 IPv4/IPv6 互通机制、**IPv4-mapped** 地址、**`IPV6_V6ONLY`**、**`IN6_IS_ADDR_V4MAPPED`** 与协议无关可移植编码范式。

---

## 小节索引

| 节 | 目录 | 主题 |
|----|------|------|
| 12.1 | [12.1_Overview](./12.1_Overview.md) | 过渡背景、双栈 |
| 12.2 | [12.2_IPv4_Client_IPv6_Server](./12.2_IPv4_Client_IPv6_Server.md) | **IPv4 客 → IPv6 服** |
| 12.3 | [12.3_IPv6_Client_IPv4_Server](./12.3_IPv6_Client_IPv4_Server.md) | **IPv6 客 → IPv4 服** |
| 12.4 | [12.4_IPv6_Address_Macro](./12.4_IPv6_Address_Macro.md) | **V4MAPPED 宏** |
| 12.5 | [12.5_Source_Code_Portability](./12.5_Source_Code_Portability.md) | 可移植黄金法则 |
| 12.6 | [12.6_Summary](./12.6_Summary.md) | 全章收束 |

---

## 一章速记

```text
IPv6 服 bind :: + 未设 V6ONLY → 同时接 IPv4/IPv6（内核 ::ffff: 映射）
IPv6 客 + AI_V4MAPPED → connect 映射地址 → 内核发 IPv4 SYN
纯 IPv6 客不能直连纯 IPv4 服（要 NAT64/DNS64）
IN6_IS_ADDR_V4MAPPED → 从 sockaddr_in6 还原真 IPv4
不写死 AF_*；getaddrinfo + sockaddr_storage + getnameinfo
```

---

## 与前后章挂钩

| 章节 | 关联 |
|------|------|
| Ch 3 | `sockaddr_storage`、`sockaddr_in6` |
| Ch 7.8 | **`IPV6_V6ONLY`** |
| Ch 11 | **`getaddrinfo`**、**`AI_V4MAPPED`** |
| [Ch 15](../Chapter15_UnixDomainProtocol/study.md) | **AF_LOCAL** IPC、传 fd |
| Ch 21 | IPv6 多播接口 |
