# 第 5 章：TCP 客户/服务器程序示例

> [Ch 4](../Chapter04_BasicTCPSocket/study.md) → **Ch 5（厚版笔记）** → [Ch 6](../Chapter06_IO_Select_Poll/study.md)  
> 每节正文见 `5.x_*.md`（含核心主旨、细节、逻辑脉络、易错点、留白）

## 本章目标

在可运行的 **TCP Echo** 上实现**生产级健壮性**：进程/信号/accept 异常/主机故障/数据格式；并认清**阻塞 I/O 天花板**，引出多路复用。

---

## 小节索引

| 节 | 目录 | 一句话 |
|----|------|--------|
| 5.1 | [5.1_Overview](./5.1_Overview.md) | Echo 架构与边界条件总览 |
| 5.2 | [5.2_Server_Main](./5.2_Server_Main.md) | fork 并发 main；父关 connfd |
| 5.3 | [5.3_Server_Str_Echo](./5.3_Server_Str_Echo.md) | read + writen 回射 |
| 5.4 | [5.4_Client_Main](./5.4_Client_Main.md) | inet_pton + Connect |
| 5.5 | [5.5_Client_Str_Cli](./5.5_Client_Str_Cli.md) | fgets/writen/readline；stdin 阻塞缺陷 |
| 5.6 | [5.6_Normal_Start](./5.6_Normal_Start.md) | netstat/ps 观测 |
| 5.7 | [5.7_Normal_Exit](./5.7_Normal_Exit.md) | 四次挥手；僵尸 |
| 5.8 | [5.8_POSIX_Signal](./5.8_POSIX_Signal.md) | sigaction；SA_RESTART |
| 5.9 | [5.9_SIGCHLD_Process](./5.9_SIGCHLD_Process.md) | 注册 SIGCHLD |
| 5.10 | [5.10_Wait_Waitpid_Func](./5.10_Wait_Waitpid_Func.md) | while WNOHANG 铁律 |
| 5.11 | [5.11_Accept_Interrupted](./5.11_Accept_Interrupted.md) | ECONNABORTED continue |
| 5.12 | [5.12_Server_Process_Abort](./5.12_Server_Process_Abort.md) | kill 子进程；感知延迟 |
| 5.13 | [5.13_SIGPIPE_Signal](./5.13_SIGPIPE_Signal.md) | SIG_IGN |
| 5.14 | [5.14_Server_Host_Crash](./5.14_Server_Host_Crash.md) | ETIMEDOUT；黑洞 |
| 5.15 | [5.15_Server_Host_Restart](./5.15_Server_Host_Restart.md) | ECONNRESET |
| 5.16 | [5.16_Server_Host_Shutdown](./5.16_Server_Host_Shutdown.md) | SIGTERM/KILL ≈ 5.12 |
| 5.17 | [5.17_TCP_Demo_Summary](./5.17_TCP_Demo_Summary.md) | 四条铁律清单 |
| 5.18 | [5.18_Data_Format_Transfer](./5.18_Data_Format_Transfer.md) | 勿裸发 struct |
| 5.19 | [5.19_Summary](./5.19_Summary.md) | 全章收束 → Ch6 |

---

## 一章速记（复习用，细节见各节 notes）

```text
Echo：stdin→writen→readline；服：read+writen。
fork：子关listen 父关connfd；SIGCHLD+while(waitpid WNOHANG)。
accept ECONNABORTED→continue；SIGPIPE→SIG_IGN。
进程死：FIN但fgets不知；再写→RST→readline=0。
主机死：ETIMEDOUT；重启：RST/ECONNRESET。
勿write(struct)；阻塞I/O→Ch6 select。
```

---

## 异常四象限（速查）

| 底端信令 | 典型 errno/现象 | 小节 |
|----------|-----------------|------|
| FIN | read=0；可能延迟 | 5.7、5.12、5.16 |
| RST | ECONNRESET；SIGPIPE/EPIPE | 5.12、5.13、5.15 |
| 超时/不可达 | ETIMEDOUT、EHOSTUNREACH | 5.14 |
| 僵尸 | ps Z | 5.7、5.10 |
