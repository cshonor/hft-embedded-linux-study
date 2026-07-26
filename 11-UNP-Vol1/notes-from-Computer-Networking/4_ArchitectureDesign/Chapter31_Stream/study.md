# 第 31 章：流 STREAMS（厚版）

> [Ch 30 服务器范式](../Chapter30_Client_Server_DesignMode/study.md) · **Ch 31**（`4_ArchitectureDesign`）· 全书架构层收束  
> 逐节：`31.x_*.md`

> **说明**：上传资料截至第 8 章；第 31 章框架来自目录（约第 14 页），细节按 UNP 第 3 版整理，请与全本对照验证。

## 本章目标

理解 **STREAMS 栈结构**、**putmsg/getmsg**、**优先级频带**、**I_PUSH/I_SETSIG**、**TPI** 与 Berkeley **Sockets** 的分歧及现代淘汰现状。

---

## 小节索引

| 节 | 目录 | 主题 |
|----|------|------|
| 31.1 | [31.1_Overview](./31.1_Overview.md) | SVR vs Berkeley |
| 31.2 | [31.2_Stream_Structure_Profile](./31.2_Stream_Structure_Profile.md) | 流头/模块/驱动、M_DATA |
| 31.3 | [31.3_Getmsg_Putmsg_Func](./31.3_Getmsg_Putmsg_Func.md) | **putmsg/getmsg** |
| 31.4 | [31.4_Getpmsg_Putpmsg_Func](./31.4_Getpmsg_Putpmsg_Func.md) | **putpmsg/getpmsg** |
| 31.5 | [31.5_Ioctl_Stream_Control](./31.5_Ioctl_Stream_Control.md) | **I_PUSH**、**SIGPOLL** |
| 31.6 | [31.6_Transport_Provider_Interface](./31.6_Transport_Provider_Interface.md) | **TPI**、socket 封装 |
| 31.7 | [31.7_Summary](./31.7_Summary.md) | 全章收束 |

---

## 一章速记

```text
STREAMS：全双工消息；SVR4 网络可模块化堆叠
流头 → 可选模块(I_PUSH) → 驱动
M_DATA 数据；M_PROTO/M_PCPROTO 控制
putmsg/getmsg：strbuf 分 ctl + data
putpmsg/getpmsg：频带 0/1-255/高优先级(OOB 类)
ioctl：I_PUSH/I_POP/I_SETSIG→SIGPOLL（信号驱动 I/O）
TPI：T_BIND_REQ/T_CONN_REQ… 代替 bind/connect；libc 可伪装 socket()
Linux/FreeBSD：STREAMS 淘汰；Solaris 史、标准演进
```

---

## 与全书挂钩

| 章节 | 关联 |
|------|------|
| Ch 1.8 | BSD vs System V 分野 |
| Ch 3–4 | Berkeley socket 对照 |
| Ch 17 | ioctl 另一战场 |
| Ch 24–25 | OOB、SIGIO/SIGPOLL |
| Ch 30 | 现代服务器范式（Sockets 世界） |
