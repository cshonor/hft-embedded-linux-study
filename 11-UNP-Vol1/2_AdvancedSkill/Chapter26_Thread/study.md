# 第 26 章：线程（厚版）

> [Ch 16](../Chapter16_NonBlockingIO/study.md) · **Ch 26** · 阶段二（`2_AdvancedSkill`）  
> 逐节：`26.x_*.md`

> **说明**：上传资料截至第 8 章；第 26 章按 UNP 第 3 版体系整理，请与全本对照验证。

## 本章目标

掌握 **Pthreads** 创建/汇合/分离、线程版 str_cli 与 TCP 服、**connfd 竞态**与 **close 陷阱**、**TSD**、**Mutex**、**条件变量** 与并发 Web 客户。

---

## 小节索引

| 节 | 目录 | 主题 |
|----|------|------|
| 26.1 | [26.1_Overview](./26.1_Overview.md) | 线程 vs fork |
| 26.2 | [26.2_Thread_Create_Exit](./26.2_Thread_Create_Exit.md) | create/join/detach |
| 26.3 | [26.3_Thread_Str_Cli](./26.3_Thread_Str_Cli.md) | 双线程 str_cli |
| 26.4 | [26.4_Thread_TCP_Server](./26.4_Thread_TCP_Server.md) | 每连接一线程 |
| 26.5 | [26.5_Thread_Private_Data](./26.5_Thread_Private_Data.md) | TSD |
| 26.6 | [26.6_Web_Client_Multi_Connect](./26.6_Web_Client_Multi_Connect.md) | 并发上限 |
| 26.7 | [26.7_Mutex_Lock](./26.7_Mutex_Lock.md) | 互斥锁 |
| 26.8 | [26.8_Condition_Variable](./26.8_Condition_Variable.md) | cond_wait + while |
| 26.9 | [26.9_Web_Client_Connect_Supplement](./26.9_Web_Client_Connect_Supplement.md) | Mutex+cond 完整流 |
| 26.10 | [26.10_Summary](./26.10_Summary.md) | 全章收束 |

---

## 一章速记

```text
pthread_create / join / detach；子线程勿乱用 exit() 杀全进程
传参：malloc(connfd) 或 (void*)connfd；禁止 &connfd 栈地址
线程服：主线程不能 close(connfd)
str_cli：主写+子读；EOF→shutdown(SHUT_WR)
TSD：key_create + set/getspecific + 析构
满槽等待：mutex + while + cond_wait；子线程结束 cond_signal
```

---

## 与前面章节挂钩

| 章节 | 关联 |
|------|------|
| Ch 4–5 | fork 并发服 → 26.4 线程版 |
| Ch 6 | select str_cli → 26.3 线程版 |
| Ch 11.18 | 不可重入 → 26.5 TSD |
| Ch 16 | 非阻塞 Web 客户 → 26.6–26.9 线程+同步 |

---

## 阶段二（2_AdvancedSkill）进度

| 章 | 状态 |
|----|------|
| 11 名字与地址 | 厚版完成 |
| 13 守护进程/inetd | 厚版完成 |
| 14 高级 I/O | 厚版完成 |
| 16 非阻塞 I/O | 厚版完成 |
| **26 线程** | **厚版完成** |
