# 第 6 章：I/O 复用 — select 和 poll（厚版）

> [Ch 5](../Chapter05_TCP_Client_Server_Demo/study.md) → **Ch 6** → [Ch 7](../Chapter07_SocketOption/study.md)  
> 逐节：`6.x_*.md`（与粘贴纪要同结构的厚版）

## 本章目标

用 **select/poll** 同时监听多描述符；用 **shutdown** 半关闭解决批量输入；单进程多路复用服务器；认清 **同步 I/O** 与 **非阻塞** 必要性。

---

## 小节索引

| 节 | 目录 | 主题 |
|----|------|------|
| 6.1 | [6.1_Overview](./6.1_Overview.md) | Ch5 缺陷、多路复用场景 |
| 6.2 | [6.2_IO_Model_Type](./6.2_IO_Model_Type.md) | 五模型、同步/异步 |
| 6.3 | [6.3_Select_Function](./6.3_Select_Function.md) | select、就绪条件、1024 |
| 6.4 | [6.4_Str_Cli_Revised](./6.4_Str_Cli_Revised.md) | select 版客户 |
| 6.5 | [6.5_Batch_Input_Process](./6.5_Batch_Input_Process.md) | 重定向丢回射 |
| 6.6 | [6.6_Shutdown_Function](./6.6_Shutdown_Function.md) | SHUT_WR 半关闭 |
| 6.7 | [6.7_Str_Cli_Final_Revised](./6.7_Str_Cli_Final_Revised.md) | 终极客户 |
| 6.8 | [6.8_TCP_Server_Revised](./6.8_TCP_Server_Revised.md) | select 单进程服 |
| 6.9 | [6.9_Pselect_Function](./6.9_Pselect_Function.md) | pselect、sigmask |
| 6.10 | [6.10_Poll_Function](./6.10_Poll_Function.md) | poll、pollfd |
| 6.11 | [6.11_TCP_Server_Poll_Revised](./6.11_TCP_Server_Poll_Revised.md) | poll 服务器 |
| 6.12 | [6.12_Summary](./6.12_Summary.md) | 小结 |

---

## 一章速记

```text
五模型：阻塞/非阻塞/复用/信号/AIO；前四同步，AIO真异步。
select：maxfdp1、fd_set四宏、FIN也算读就绪；FD_SETSIZE≈1024。
客户：select(stdin,sock)；批量EOF→shutdown(SHUT_WR)继续读。
勿stdio+fgets；用read/write。
服务器：单进程+client[]；阻塞read→DoS→要非阻塞(Ch16)。
poll：无1024、events/revents、fd=-1忽略。
close双向关；shutdown可半关闭写。
```

---

## Ch5 → Ch6 对照

| 问题 | Ch5 | Ch6 |
|------|-----|-----|
| 服务器死了还在等键盘 | fgets 阻塞 | select 唤醒 sockfd |
| 文件输入完就 exit | 丢在途回射 | shutdown + 继续读 |
| 每连接 fork | 贵 | select/poll 单进程 |
