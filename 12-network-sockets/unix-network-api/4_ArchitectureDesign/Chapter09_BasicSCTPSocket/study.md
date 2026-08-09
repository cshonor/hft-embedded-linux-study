# 第 9 章：基本 SCTP 套接字编程（厚版）

> 阶段一收束：[Ch 8](../../1_BasicFoundation/Chapter08_BasicUDPSocket/study.md) · **Ch 9**（`4_ArchitectureDesign`）· [Ch 10](../Chapter10_SCTP_Client_Server_Demo/study.md)  
> 逐节：`9.x_*.md`

> **说明**：上传资料正文截至第 8 章；第 9 章框架来自目录，细节按 UNP 第 3 版整理，请与全本对照验证。

## 本章目标

掌握 SCTP **一到一/一到多** 模型、`sctp_bindx`/`connectx`、地址查询、`sctp_sendmsg`/`recvmsg`、`sctp_opt_info`、`sctp_peeloff`、`shutdown` 语义与 **Notifications**。

---

## 小节索引

| 节 | 目录 | 主题 |
|----|------|------|
| 9.1 | [9.1_Overview](./9.1_Overview.md) | SCTP 与专用 API |
| 9.2 | [9.2_Interface_Model](./9.2_Interface_Model.md) | 一到一 / 一到多 |
| 9.3 | [9.3_Sctp_Bindx_Func](./9.3_Sctp_Bindx_Func.md) | 多宿 bind |
| 9.4 | [9.4_Sctp_Connectx_Func](./9.4_Sctp_Connectx_Func.md) | 多宿 connect |
| 9.5 | [9.5_Sctp_Getpaddrs_Func](./9.5_Sctp_Getpaddrs_Func.md) | 对端地址 |
| 9.6 | [9.6_Sctp_Freepaddrs_Func](./9.6_Sctp_Freepaddrs_Func.md) | 释放对端地址 |
| 9.7 | [9.7_Sctp_Getladdrs_Func](./9.7_Sctp_Getladdrs_Func.md) | 本地地址 |
| 9.8 | [9.8_Sctp_Freeladdrs_Func](./9.8_Sctp_Freeladdrs_Func.md) | 释放本地地址 |
| 9.9 | [9.9_Sctp_Sendmsg_Func](./9.9_Sctp_Sendmsg_Func.md) | **发送核心** |
| 9.10 | [9.10_Sctp_Recvmsg_Func](./9.10_Sctp_Recvmsg_Func.md) | **接收核心** |
| 9.11 | [9.11_Sctp_Opt_Info_Func](./9.11_Sctp_Opt_Info_Func.md) | per-assoc 选项 |
| 9.12 | [9.12_Sctp_Peeloff_Func](./9.12_Sctp_Peeloff_Func.md) | 关联剥离 |
| 9.13 | [9.13_Shutdown_Func](./9.13_Shutdown_Func.md) | 无半关闭 |
| 9.14 | [9.14_SCTP_Notification](./9.14_SCTP_Notification.md) | 带内事件 |
| 9.15 | [9.15_Summary](./9.15_Summary.md) | 全章收束 |

---

## 一章速记

```text
SOCK_STREAM+SCTP：TCP 式一到一；SOCK_SEQPACKET：一到多
sctp_sendmsg/recvmsg：stream、PPID、UNORDERED、assoc_id
sctp_peeloff：长关联独立 fd；shutdown 非半关闭
SCTP_EVENTS + MSG_NOTIFICATION：关联/路径/发送失败事件
getpaddrs/getladdrs 必须 free
```

---

## 与前面章节挂钩

| 章节 | 关联 |
|------|------|
| Ch 2.5、2.8 | SCTP 协议与关联建立/终止 |
| Ch 4 | 一到一复用 listen/accept |
| Ch 7.10 | SCTP 套接字选项 |
| Ch 10 | 一到多回射与队头阻塞 |
| [Ch 23](../Chapter23_AdvancedSCTPSocket/study.md) | 高级 SCTP |
