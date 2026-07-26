# 第 10 章：SCTP 客户/服务器程序例子（厚版）

> [Ch 9](../Chapter09_BasicSCTPSocket/study.md) · **Ch 10** · [Ch 11](../../2_AdvancedSkill/Chapter11_Name_Address_Convert/study.md) · [Ch 12](../Chapter12_IPv4_IPv6_Interop/study.md)  
> 逐节：`10.x_*.md`

> **说明**：上传资料截至第 8 章；第 10 章框架来自目录，细节按 UNP 第 3 版整理，请与全本对照验证。

## 本章目标

用 **一到多 SCTP 回射** 演示 `SCTP_EVENTS`、`sctp_sendmsg/recvmsg` 多流轮询、**队头阻塞对比**、`SCTP_INITMSG` 与 **`SCTP_EOF`** 单关联关闭。

---

## 小节索引

| 节 | 目录 | 主题 |
|----|------|------|
| 10.1 | [10.1_Overview](./10.1_Overview.md) | 一到多回射总览 |
| 10.2 | [10.2_SCTP_OneToMany_Server](./10.2_SCTP_OneToMany_Server.md) | 服务器 main |
| 10.3 | [10.3_SCTP_OneToMany_Client](./10.3_SCTP_OneToMany_Client.md) | 客户 main |
| 10.4 | [10.4_Sctp_Str_Cli_Func](./10.4_Sctp_Str_Cli_Func.md) | sctpstr_cli 多流 |
| 10.5 | [10.5_Head_Blocking_Problem](./10.5_Head_Blocking_Problem.md) | **队头阻塞** |
| 10.6 | [10.6_Stream_Number_Control](./10.6_Stream_Number_Control.md) | SCTP_INITMSG |
| 10.7 | [10.7_Connection_Terminate_Control](./10.7_Connection_Terminate_Control.md) | SCTP_EOF |
| 10.8 | [10.8_Summary](./10.8_Summary.md) | 全章收束 |

---

## 一章速记

```text
SOCK_SEQPACKET 服：listen + sctp_recvmsg/sendmsg 循环，无 accept
必开 sctp_data_io_event → sndrcvinfo 有 stream/assoc_id
客：不 connect；sctpstr_cli 轮询 sinfo_stream
TCP：A 丢则 B/C 也堵；SCTP：不同流独立交付
关联前 SCTP_INITMSG；关一个客户：sendmsg SCTP_EOF，勿 close 整 fd
```

---

## 与前面章节挂钩

| 章节 | 关联 |
|------|------|
| Ch 9 | 全套 SCTP API |
| Ch 2.4–2.5 | TCP 流 vs SCTP 多流 |
| Ch 6–8 | 回射、select 客户对比 |
| [Ch 23](../Chapter23_AdvancedSCTPSocket/study.md) | 高级 SCTP 深化 |
