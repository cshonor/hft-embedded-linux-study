# 23.1 概述

> [Ch 9 SCTP API](../Chapter09_BasicSCTPSocket/study.md) · [Ch 10 回射](../Chapter10_SCTP_Client_Server_Demo/study.md) · [study.md](../study.md)

---

## 核心主旨

在 Ch 9–10 基础 API 与一到多模型之后，本章深入 SCTP **高级特性**：海量关联资源回收、超大消息**部分递送**、多宿心跳与定时器、关联剥离等。

SCTP 状态机比 TCP 更庞大，赋予开发者更高控制权。

---

## 本章路线图

```text
23.2  SCTP_AUTOCLOSE（一到多僵死关联）
23.3  部分递送 + MSG_EOR
23.4  高级通知类型
23.5  SCTP_UNORDERED
23.6  sctp_bindx 地址子集
23.7–23.8  getpaddrs / getladdrs / sctp_opt_info
23.9  心搏与路径不可达
23.10 sctp_peeloff
23.11 RTO / 关联定时器
23.12 何时用 SCTP 代替 TCP
```

---

## 资料说明

> 上传资料截至第 8 章；第 23 章框架来自目录（约第 13 页），细节按 UNP 第 3 版整理，请与全本对照验证。

---

## 个人学习总结

（待填）
