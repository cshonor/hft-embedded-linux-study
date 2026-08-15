# 第 14 章：高级 I/O 函数（厚版）

> [Ch 13](../Chapter13_Daemon_Inetd/study.md) · **Ch 14** · [Ch 15](../../4_ArchitectureDesign/Chapter15_UnixDomainProtocol/)（待笔记）  
> 逐节：`14.x_*.md`

> **说明**：上传资料截至第 8 章；第 14 章按 UNP 第 3 版体系整理，请与全本对照验证。

## 本章目标

掌握套接字**超时**三法、`recv/send` 标志、`readv/writev`、`recvmsg/sendmsg` 与**辅助数据**、排队探测、**stdio 陷阱**、高级轮询与 T/TCP 背景。

---

## 小节索引

| 节 | 目录 | 主题 |
|----|------|------|
| 14.1 | [14.1_Overview](./14.1_Overview.md) | 章概览 |
| 14.2 | [14.2_Socket_Timeout_Set](./14.2_Socket_Timeout_Set.md) | alarm / select / SO_RCVTIMEO |
| 14.3 | [14.3_Recv_Send_Func](./14.3_Recv_Send_Func.md) | flags |
| 14.4 | [14.4_Readv_Writev_Func](./14.4_Readv_Writev_Func.md) | 聚集/分散 I/O |
| 14.5 | [14.5_Recvmsg_Sendmsg_Func](./14.5_Recvmsg_Sendmsg_Func.md) | msghdr |
| 14.6 | [14.6_Auxiliary_Data](./14.6_Auxiliary_Data.md) | cmsghdr 宏 |
| 14.7 | [14.7_Pending_Data_Check](./14.7_Pending_Data_Check.md) | FIONREAD 等 |
| 14.8 | [14.8_Socket_StdIO_Mix](./14.8_Socket_StdIO_Mix.md) | 勿混 stdio |
| 14.9 | [14.9_Advanced_Poll_Method](./14.9_Advanced_Poll_Method.md) | kqueue/devpoll/epoll |
| 14.10 | [14.10_TCP_Transaction_Type](./14.10_TCP_Transaction_Type.md) | T/TCP（历史） |
| 14.11 | [14.11_Summary](./14.11_Summary.md) | 全章收束 |

---

## 一章速记

```text
超时：首选 select/poll；SO_RCVTIMEO→EWOULDBLOCK；alarm 慎用
recv/send：MSG_PEEK/DONTWAIT/WAITALL/OOB
writev：HTTP 头+体一次 syscall
recvmsg：iovec+地址+辅助数据；用 CMSG_* 宏
勿 fdopen+select；排队字节用 FIONREAD
高并发：epoll/kqueue 替代 O(N) select
```

---

## 与前面章节挂钩

| 章节 | 关联 |
|------|------|
| Ch 3 | readn ↔ MSG_WAITALL |
| Ch 6–7 | select 超时、SO_RCVTIMEO |
| Ch 8 | recvfrom 并入 recvmsg |
| Ch 13 | inetd 仍用 select |
| Ch 15 | 辅助数据传递 fd |
| Ch 16 | 非阻塞与 FIONREAD |
| Ch 24 | MSG_OOB |
