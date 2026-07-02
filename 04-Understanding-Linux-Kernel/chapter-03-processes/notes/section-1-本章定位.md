## 1. 本章定位

> **ULK Ch 3 Processes** · 如何抽象、管理、切换、销毁进程/线程

---

### 一、本章讲什么

Ch 1 介绍了进程概念；Ch 2 讲了地址怎么翻译。本章回答：

- 内核用什么数据结构**表示**一个进程？（`task_struct`）
- 进程如何**组织、查找、睡眠、唤醒**？
- **上下文切换**在硬件和软件上怎么做？
- **fork/COW/exit** 的创建与销毁路径

全书非常核心的一章 — 读调度（Ch 7）、syscall（Ch 10）、信号（Ch 11）都依赖本章。

---

### 二、小节导航

| 节 | 主题 |
|----|------|
| [2](./section-2-进程与线程.md) | 进程、轻量级进程、线程组 |
| [3](./section-3-进程描述符.md) | `task_struct`、状态、`thread_info` |
| [4](./section-4-组织与查找.md) | 等待队列、PID 哈希表 |
| [5](./section-5-进程切换.md) | 上下文切换、`switch_to`、FPU 惰性保存 |
| [6](./section-6-创建与销毁.md) | `clone`/`fork`、COW、内核线程、僵尸 |

---

### 三、在 Linux 链上的位置

```
Ch 2  内存寻址     — COW 依赖分页
Ch 3  进程（本章） — task_struct、fork、切换
Ch 7  进程调度     — 谁下一个上 CPU
Ch 9  进程地址空间 — 每个 task 的 VMA/页表
Ch 10 系统调用     — fork/exit/wait 入口
```

交叉：[05 LKD Ch 3](../../../03-Linux-Kernel-Development/) · [01 CSAPP](../../../01-CSAPP-3rd/) Ch 8 · [08 TLPI](../../../06-The-Linux-Programming-Interface/)

---

← [Ch 3 导读](../README.md) · 下一节 [2. 进程与线程](./section-2-进程与线程.md)
