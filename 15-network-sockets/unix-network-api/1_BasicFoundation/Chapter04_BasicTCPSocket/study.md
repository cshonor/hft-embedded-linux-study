# 第 4 章：基本 TCP 套接字编程

> [Ch 3](../Chapter03_SocketProgramIntro/study.md) → **Ch 4（厚版）** → [Ch 5](../Chapter05_TCP_Client_Server_Demo/study.md)  
> 逐节：`4.x_*.md`

## 本章目标

掌握 TCP 客户/服务器 **API 矩阵**、**双队列 backlog**、**listenfd/connfd**、**fork 与引用计数**。

---

## 小节索引

| 节 | 目录 |
|----|------|
| 4.1 | [4.1_Overview](./4.1_Overview.md) |
| 4.2 | [4.2_Socket_Function](./4.2_Socket_Function.md) |
| 4.3 | [4.3_Connect_Function](./4.3_Connect_Function.md) |
| 4.4 | [4.4_Bind_Function](./4.4_Bind_Function.md) |
| 4.5 | [4.5_Listen_Function](./4.5_Listen_Function.md) |
| 4.6 | [4.6_Accept_Function](./4.6_Accept_Function.md) |
| 4.7 | [4.7_Fork_Exec_Function](./4.7_Fork_Exec_Function.md) |
| 4.8 | [4.8_ConcurrentServer](./4.8_ConcurrentServer.md) |
| 4.9 | [4.9_Close_Function](./4.9_Close_Function.md) |
| 4.10 | [4.10_Getsockname_Getpeername](./4.10_Getsockname_Getpeername.md) |
| 4.11 | [4.11_Summary](./4.11_Summary.md) |

---

## 一章速记

```text
客户 socket→connect；服 socket→bind→listen→accept。
listen 两队列；满丢 SYN。
accept 返新 connfd；listenfd 唯一。
fork：子关 listen 父关 conn；close 减引用为 0 才 FIN。
connect 失败勿重试同 fd；ECONNREFUSED=RST。
```
