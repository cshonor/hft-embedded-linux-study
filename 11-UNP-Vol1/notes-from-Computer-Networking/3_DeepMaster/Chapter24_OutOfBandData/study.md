# 第 24 章：带外数据（厚版）

> [Ch 22](../Chapter22_AdvancedUDPSocket/study.md) · **Ch 24** · [Ch 25](../Chapter25_SignalDriveIO/study.md)  
> 逐节：`24.x_*.md`

> **说明**：上传资料截至第 8 章；第 24 章按 UNP 第 3 版体系整理，请与全本对照验证。

## 本章目标

理解 TCP **紧急模式**（URG/指针）、**SO_OOBINLINE** 双读模式、**sockatmark**、RFC 793/1122 差异、**OOB 心搏** 及现代协议中的弃用趋势。

---

## 小节索引

| 节 | 目录 | 主题 |
|----|------|------|
| 24.1 | [24.1_Overview](./24.1_Overview.md) | OOB vs TCP 紧急模式 |
| 24.2 | [24.2_TCP_OutOfBand_Data](./24.2_TCP_OutOfBand_Data.md) | 发送/接收、SIGURG |
| 24.3 | [24.3_Sockatmark_Func](./24.3_Sockatmark_Func.md) | 线内模式探测 |
| 24.4 | [24.4_TCP_OutOfBand_Summary](./24.4_TCP_OutOfBand_Summary.md) | RFC、覆盖陷阱 |
| 24.5 | [24.5_Client_Server_Heartbeat](./24.5_Client_Server_Heartbeat.md) | OOB 心搏 |
| 24.6 | [24.6_Summary](./24.6_Summary.md) | 全章收束 |

---

## 一章速记

```text
TCP 无真 OOB 通道：URG + 紧急指针，通常仅最后一字节
通知：SIGURG（F_SETOWN）或 select exceptset
SO_OOBINLINE=0：recv MSG_OOB；=1：sockatmark + 普通 read
标记唯一：连续 OOB 会覆盖；read 不跨越标记
实用：Telnet、OOB 心跳（窗口为0仍可能发出）
现代应用层协议少用 TCP OOB
```

---

## 与前面章节挂钩

| 章节 | 关联 |
|------|------|
| Ch 6 | select 异常集 |
| Ch 7 | SO_OOBINLINE、SO_KEEPALIVE |
| Ch 14 | MSG_OOB |
| Ch 17 | SIOCATMARK |
| Ch 5 | SIGURG、SIGALRM 心搏 |

---

## 3_DeepMaster 进度（部分）

| 章 | 状态 |
|----|------|
| 17、20–22、**24** | 厚版完成 |
| 18、23、25、28–29 | 待笔记 |
