# 第 23 章：高级 SCTP 套接字编程（厚版）

> [Ch 9](../Chapter09_BasicSCTPSocket/study.md) · [Ch 10](../Chapter10_SCTP_Client_Server_Demo/study.md) · **Ch 23**（`4_ArchitectureDesign`）  
> 逐节：`23.x_*.md`

> **说明**：上传资料截至第 8 章；第 23 章框架来自目录（约第 13 页），细节按 UNP 第 3 版整理，请与全本对照验证。

## 本章目标

掌握 **SCTP_AUTOCLOSE**、**部分递送**、高级**通知**、**SCTP_UNORDERED**、**sctp_bindx** 子集、地址/assoc 查询、**心搏**、**sctp_peeloff**、定时器调优及 **SCTP vs TCP** 选型。

---

## 小节索引

| 节 | 目录 | 主题 |
|----|------|------|
| 23.1 | [23.1_Overview](./23.1_Overview.md) | 高级特性总览 |
| 23.2 | [23.2_AutoClose_OneToMany_Server](./23.2_AutoClose_OneToMany_Server.md) | **SCTP_AUTOCLOSE** |
| 23.3 | [23.3_Partial_Data_Deliver](./23.3_Partial_Data_Deliver.md) | **MSG_EOR**、锁定 |
| 23.4 | [23.4_SCTP_Notification_Msg](./23.4_SCTP_Notification_Msg.md) | 高级通知 |
| 23.5 | [23.5_Unordered_Data_Transfer](./23.5_Unordered_Data_Transfer.md) | **SCTP_UNORDERED** |
| 23.6 | [23.6_Bind_Address_Subset](./23.6_Bind_Address_Subset.md) | **sctp_bindx** 子集 |
| 23.7 | [23.7_Local_Remote_Addr_Query](./23.7_Local_Remote_Addr_Query.md) | getpaddrs / getladdrs |
| 23.8 | [23.8_IP_Association_ID_Match](./23.8_IP_Association_ID_Match.md) | **assoc_id** |
| 23.9 | [23.9_Heartbeat_Addr_Unreachable](./23.9_Heartbeat_Addr_Unreachable.md) | 心搏、不可达 |
| 23.10 | [23.10_Association_Split_Operate](./23.10_Association_Split_Operate.md) | **sctp_peeloff** |
| 23.11 | [23.11_Time_Parameter_Control](./23.11_Time_Parameter_Control.md) | RTO、ASSOCINFO |
| 23.12 | [23.12_SCTP_TCP_Scene_Choice](./23.12_SCTP_TCP_Scene_Choice.md) | 选型指南 |
| 23.13 | [23.13_Summary](./23.13_Summary.md) | 全章收束 |

---

## 一章速记

```text
一到多：SCTP_AUTOCLOSE 清僵死关联；peeloff 长连接独立 fd
大消息：无 MSG_EOR = 部分递送；完成前套接字锁定
通知：ASSOC_CHANGE / PEER_ADDR_CHANGE / SEND_FAILED
SCTP_UNORDERED = 可靠无序；sctp_bindx 限制对外多宿 IP
getpaddrs/getladdrs；sctp_opt_info → assoc_id
心搏 + SCTP_PEER_ADDR_PARAMS；RTOINFO / ASSOCINFO 调时
选型：多流、多宿、消息边界、UNORDERED；公网慎 NAT
```

---

## 与前后章挂钩

| 章节 | 关联 |
|------|------|
| Ch 2.5 / 7.10 | SCTP 协议与套接字选项 |
| Ch 9–10 | 基础 API、一到多回射、队头阻塞 |
| Ch 26 | peeloff 后工作线程 |
| Ch 31 | 流控相关（若已学） |
