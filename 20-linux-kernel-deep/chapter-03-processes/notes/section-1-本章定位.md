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

交叉：[05 LKD Ch 3](../../../05-linux-kernel/) · [01 CSAPP](../../../02-computer-systems/) Ch 8 · [08 TLPI](../../../03-linux-userspace-api/)

### 常见陷阱

1. 把 ULK 讲的 `task_struct` 字段当现代版——6.x 的 `task_struct` 已超过 8000 字节，字段布局和 ULK 时代完全不同
2. 混淆「线程」和「进程」在内核层面的区别——内核不区分，都是 `task_struct`，线程是共享地址空间的 `task_struct`
3. 以为 `current` 宏在所有架构上一样——x86 用 `per_cpu` 变量，ARM64 用 `sp_el0` 寄存器存储

---

<details>
<summary>自测题（点击展开）</summary>

**Q1.** 内核如何区分「进程」和「线程」？

<details><summary>答案</summary>

不区分。每个 `task_struct` 都是一个「内核可调度实体」。进程 = 独立地址空间的 `task_struct`；线程 = 共享 `mm_struct` 的 `task_struct`。`clone(CLONE_VM | CLONE_FILES | CLONE_SIGHAND, ...)` 创建线程，`clone(SIGCHLD, ...)` 创建进程。`task_struct->mm` 指向共享的 `mm_struct`，内核线程 `mm` 为 NULL。

</details>

**Q2.** `current` 宏在 x86-64 和 ARM64 上实现有何不同？

<details><summary>答案</summary>

x86-64：`current` 从 per-CPU 变量 `current_task` 读取，通过 `gs` 段寄存器基址偏移访问。ARM64：`current` 存在 `sp_el0` 寄存器中（内核态 SP_EL0 存 `task_struct` 指针），直接 `mrs x0, sp_el0` 读取。ARM64 方式更快（零内存访问），但 `sp_el0` 在内核态被复用。

</details>

**Q3.** HFT 为什么要把交易线程和内核线程分开？

<details><summary>答案</summary>

内核线程（kworker/softirq）可能抢占交易线程的 CPU。HFT 做法：① 交易线程绑独立核（`sched_setaffinity`）+ `SCHED_FIFO` 实时优先级；② `isolcpus` 隔离该核不让普通任务调度；③ 中断重定向到其他核（`/proc/irq/*/smp_affinity`）。

</details>

</details>

---

← [Ch 3 导读](../README.md) · 下一节 [2. 进程与线程](./section-2-进程与线程.md)
