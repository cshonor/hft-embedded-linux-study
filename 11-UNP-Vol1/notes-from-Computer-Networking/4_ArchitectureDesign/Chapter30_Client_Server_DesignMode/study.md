# 第 30 章：客户/服务器程序设计范式（厚版）

> [Ch 26 线程](../../2_AdvancedSkill/Chapter26_Thread/study.md) · [Ch 15 传 fd](../Chapter15_UnixDomainProtocol/study.md) · **Ch 30**（`4_ArchitectureDesign`）· [Ch 31](../Chapter31_Stream/study.md)  
> 逐节：`30.x_*.md`

> **说明**：上传资料截至第 8 章；第 30 章框架来自目录，细节按 UNP 第 3 版整理，请与全本对照验证。

## 本章目标

横向对比 **9 种 TCP 服务器架构**（迭代、fork、Prefork 四变体、每连接线程、Prethread 两变体），理解惊群、锁、传 fd 与**池化**在压测中的权衡。

---

## 小节索引

| 节 | 目录 | 主题 |
|----|------|------|
| 30.1 | [30.1_Overview](./30.1_Overview.md) | 九范式总览 |
| 30.2 | [30.2_TCP_Client_Design_Pattern](./30.2_TCP_Client_Design_Pattern.md) | 压测客户范式 |
| 30.3 | [30.3_TCP_Test_Client](./30.3_TCP_Test_Client.md) | 测试程序 |
| 30.4 | [30.4_TCP_Iterative_Server](./30.4_TCP_Iterative_Server.md) | **迭代** |
| 30.5 | [30.5_TCP_Fork_Concurrent_Server](./30.5_TCP_Fork_Concurrent_Server.md) | **每连接 fork** |
| 30.6 | [30.6_PreFork_Server_NoLock](./30.6_PreFork_Server_NoLock.md) | Prefork、**惊群** |
| 30.7 | [30.7_PreFork_Server_FileLock](./30.7_PreFork_Server_FileLock.md) | 文件锁 accept |
| 30.8 | [30.8_PreFork_Server_ThreadLock](./30.8_PreFork_Server_ThreadLock.md) | **共享互斥锁** |
| 30.9 | [30.9_PreFork_Server_Fd_Transfer](./30.9_PreFork_Server_Fd_Transfer.md) | **SCM_RIGHTS** |
| 30.10 | [30.10_TCP_Thread_Concurrent_Server](./30.10_TCP_Thread_Concurrent_Server.md) | 每连接线程 |
| 30.11 | [30.11_PreThread_Server_SingleAccept](./30.11_PreThread_Server_SingleAccept.md) | 线程抢 accept |
| 30.12 | [30.12_PreThread_Server_MainAccept](./30.12_PreThread_Server_MainAccept.md) | 主 accept + 队列 |
| 30.13 | [30.13_Summary](./30.13_Summary.md) | 选型排名 |

---

## 一章速记

```text
压测：数千并发短连接回射
迭代：无并发；fork/连接：简单但 fork 贵
Prefork：预派 N 子进程
  30.6 全 accept → 惊群；30.7 文件锁慢
  30.8 mmap 互斥锁 → 多进程最优、Nginx 思想
  30.9 主 accept + UDS 传 fd → 优雅略慢
每连接 pthread：快、不隔离、要线程安全库
Prethread：30.11 线程锁 accept → 吞吐之王
           30.12 主 accept + 条件变量队列 → 易调度
短连接海量 → 池化；长连接稀疏 → fork/连接可够
```

---

## 与前后章挂钩

| 章节 | 关联 |
|------|------|
| Ch 4–5 | accept、fork、回射 |
| Ch 6 | 事件驱动（本章未覆盖 select/epoll 服，但可对比） |
| Ch 15 | 30.9 描述符传递 |
| Ch 26 | 线程、互斥锁、条件变量 |
| [Ch 31](../Chapter31_Stream/study.md) | **STREAMS** / TPI（SVR4 史） |
