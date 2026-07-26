# 第 8 章：基本 UDP 套接字编程（厚版）

> [Ch 7](../Chapter07_SocketOption/study.md) → **Ch 8** → 阶段二 Ch9+  
> 逐节：`8.x_*.md`

> **说明**：若教材 PDF 仅含 8.1，8.2～8.16 笔记按 UNP 第 3 版体系整理，请与书本对照。

## 本章目标

掌握 **recvfrom/sendto**、UDP **迭代服务器**、丢包/验源/ICMP 异步错误、**UDP connect**、与 **select 混服 TCP+UDP**。

---

## 小节索引

| 节 | 目录 | 主题 |
|----|------|------|
| 8.1 | [8.1_Overview](./8.1_Overview.md) | 无连接时序 |
| 8.2 | [8.2_Recvfrom_Sendto_Func](./8.2_Recvfrom_Sendto_Func.md) | API、0 字节≠EOF |
| 8.3 | [8.3_UDP_Server_Main](./8.3_UDP_Server_Main.md) | 服务器 main |
| 8.4 | [8.4_UDP_Server_Dg_Echo](./8.4_UDP_Server_Dg_Echo.md) | 迭代 dg_echo |
| 8.5 | [8.5_UDP_Client_Main](./8.5_UDP_Client_Main.md) | 客户 main |
| 8.6 | [8.6_UDP_Client_Dg_Cli](./8.6_UDP_Client_Dg_Cli.md) | dg_cli |
| 8.7 | [8.7_Datagram_Loss_Problem](./8.7_Datagram_Loss_Problem.md) | 须超时 |
| 8.8 | [8.8_Response_Data_Verify](./8.8_Response_Data_Verify.md) | 校验来源 |
| 8.9 | [8.9_Server_Offline_State](./8.9_Server_Offline_State.md) | ICMP、异步错误 |
| 8.10 | [8.10_UDP_Demo_Summary](./8.10_UDP_Demo_Summary.md) | 阶段性小结 |
| 8.11 | [8.11_UDP_Connect_Usage](./8.11_UDP_Connect_Usage.md) | **UDP connect** |
| 8.12 | [8.12_Dg_Cli_Revised](./8.12_Dg_Cli_Revised.md) | read/write 版 |
| 8.13 | [8.13_UDP_FlowControl_Defect](./8.13_UDP_FlowControl_Defect.md) | 无流控丢包 |
| 8.14 | [8.14_UDP_Outbound_Interface](./8.14_UDP_Outbound_Interface.md) | 源 IP/路由 |
| 8.15 | [8.15_TCP_UDP_Mixed_Server](./8.15_TCP_UDP_Mixed_Server.md) | select 混服 |
| 8.16 | [8.16_Summary](./8.16_Summary.md) | 全章收束 |

---

## 一章速记

```text
服：socket→bind→recvfrom/sendto循环，无listen/accept。
客：sendto+地址→recvfrom；0字节UDP≠TCP的EOF。
勿永久recvfrom：超时/重传。
验源地址或UDP connect→read/write+ECONNREFUSED。
无流控：接收满则丢，应用层ACK限流。
同端口TCP+UDP可select分流。
```

---

## 与 Ch5～7 挂钩

| 问题 | Ch8 解法 |
|------|----------|
| recvfrom 死锁 | 7 SO_RCVTIMEO、6 select 超时 |
| 假响应 | 8.8 验源 / 8.11 connect |
| 端口无进程 | 8.9 ICMP → 8.11 connect 后 read 错 |
| 广播 | 7 SO_BROADCAST，勿对 TCP 开 |
