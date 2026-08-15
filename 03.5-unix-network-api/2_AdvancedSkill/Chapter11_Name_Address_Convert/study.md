# 第 11 章：名字与地址转换（厚版）

> 阶段一：[Ch 8](../../1_BasicFoundation/Chapter08_BasicUDPSocket/study.md) · **Ch 11**（`2_AdvancedSkill`）  
> 逐节：`11.x_*.md`

> **说明**：上传资料正文截至第 8 章；第 11 章按目录与 UNP 第 3 版体系整理，请与教材对照验证。

## 本章目标

掌握 **DNS 基础**、**getaddrinfo/getnameinfo** 协议无关解析，以及 **tcp_connect / tcp_listen / udp_*** 封装模式；理解旧 API 的可重入陷阱。

---

## 小节索引

| 节 | 目录 | 主题 |
|----|------|------|
| 11.1 | [11.1_Overview](./11.1_Overview.md) | 名字与协议无关 |
| 11.2 | [11.2_DNS_System](./11.2_DNS_System.md) | A/AAAA/PTR/MX/CNAME |
| 11.3 | [11.3_Gethostbyname_Func](./11.3_Gethostbyname_Func.md) | 旧 API（勿用） |
| 11.4 | [11.4_Gethostbyaddr_Func](./11.4_Gethostbyaddr_Func.md) | 反向解析（旧） |
| 11.5 | [11.5_Getservbyname_Getservbyport](./11.5_Getservbyname_Getservbyport.md) | /etc/services |
| 11.6 | [11.6_Getaddrinfo_Func](./11.6_Getaddrinfo_Func.md) | **核心 API** |
| 11.7 | [11.7_Gai_strerror_Func](./11.7_Gai_strerror_Func.md) | 错误串 |
| 11.8 | [11.8_Freeaddrinfo_Func](./11.8_Freeaddrinfo_Func.md) | 释放链表 |
| 11.9 | [11.9_Getaddrinfo_IPv6](./11.9_Getaddrinfo_IPv6.md) | AI_V4MAPPED/ALL |
| 11.10 | [11.10_Getaddrinfo_CaseDemo](./11.10_Getaddrinfo_CaseDemo.md) | 客户/服例子 |
| 11.11 | [11.11_Host_Serv_Func](./11.11_Host_Serv_Func.md) | hints 封装 |
| 11.12 | [11.12_Tcp_Connect_Func](./11.12_Tcp_Connect_Func.md) | TCP 客户 |
| 11.13 | [11.13_Tcp_Listen_Func](./11.13_Tcp_Listen_Func.md) | TCP 监听 |
| 11.14 | [11.14_Udp_Client_Func](./11.14_Udp_Client_Func.md) | UDP 客户 |
| 11.15 | [11.15_Udp_Connect_Func](./11.15_Udp_Connect_Func.md) | 已连接 UDP |
| 11.16 | [11.16_Udp_Server_Func](./11.16_Udp_Server_Func.md) | UDP 服 |
| 11.17 | [11.17_Getnameinfo_Func](./11.17_Getnameinfo_Func.md) | 反向（新） |
| 11.18 | [11.18_Reentrant_Function](./11.18_Reentrant_Function.md) | 可重入 |
| 11.19 | [11.19_Gethostbyname_r_Gethostbyaddr_r](./11.19_Gethostbyname_r_Gethostbyaddr_r.md) | _r 变体 |
| 11.20 | [11.20_Old_IPv6_Convert_Func](./11.20_Old_IPv6_Convert_Func.md) | 作废 API |
| 11.21 | [11.21_Other_Network_Info](./11.21_Other_Network_Info.md) | 网络/协议 DB |
| 11.22 | [11.22_Summary](./11.22_Summary.md) | 全章收束 |

---

## 一章速记

```text
解析：getaddrinfo(host, service, hints, &res) → 遍历 ai_next
      socket/connect 或 bind/listen；用完 freeaddrinfo
服务端 bind：hostname=NULL + AI_PASSIVE
展示对端：getnameinfo；调试可用 NI_NUMERICHOST 免 DNS
禁用：gethostbyname/addr、inet_ntoa（静态缓冲）
封装：tcp_connect / tcp_listen / udp_client / udp_connect / udp_server
```

---

## API 对照

| 需求 | 现代 | 过时 |
|------|------|------|
| 主机名→地址 | getaddrinfo | gethostbyname |
| 地址→主机名 | getnameinfo | gethostbyaddr |
| 服务名↔端口 | getaddrinfo 的 service | getservby* |
| 错误信息 | gai_strerror | h_errno（旧） |

---

## 与 Ch 3～8 挂钩

| 章节 | 关联 |
|------|------|
| Ch 3 | `sockaddr`、字节序 → `ai_addr` 已就绪 |
| Ch 4 | `connect`/`bind` → 对链表每一项尝试 |
| Ch 7 | `tcp_listen` 中 SO_REUSEADDR |
| Ch 8 | `udp_connect` 与 11.15 一致 |
| Ch 12 | 双栈互操作、`AI_V4MAPPED`、mapped 地址 |
