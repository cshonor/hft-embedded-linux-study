## 2. 进程、轻量级进程与线程

---

### 一、进程是什么

**进程** = 「执行中程序的实例」— OS 调度和资源分配的基本单位。

---

### 二、Linux 的 LWP 模型

Linux **没有**像传统 Unix 那样严格区分「进程 vs 线程」，而是引入 **轻量级进程（LWP, Lightweight Process）**：

| 特点 | 说明 |
|------|------|
| 多线程应用 | 由**一组 LWP** 组成 |
| 共享资源 | 同一组内共享 **内存地址空间**、打开文件等 |
| 独立调度 | 每个 LWP 仍可被内核单独调度（modern 下即 `task_struct`） |

→ POSIX 线程（pthread）在 Linux 上底层就是 **clone + 共享 VM**。

---

### 三、线程组（Thread Group）与 PID

POSIX 要求：**同一多线程应用内所有线程共享同一个 PID**。

Linux 做法：

| 字段 | 含义 |
|------|------|
| **PID** | 每个 LWP 仍有唯一内核 PID |
| **`tgid`（Thread Group ID）** | 线程组 ID = **领头线程的 PID** |
| `getpid()` | 用户看到的其实是 **`tgid`**，不是单个 LWP 的 pid |

这样对外符合 POSIX，对内仍可独立调度每个线程。

---

### 四、和后续章的关系

| 主题 | 章节 |
|------|------|
| `task_struct` 细节 | [section-3](./section-3-进程描述符.md) |
| `clone()` 创建线程 | [section-6](./section-6-创建与销毁.md) |
| 线程调度 | [Ch 7 进程调度](../../chapter-07-process-scheduling.md) |

### 常见陷阱

1. 以为 `fork()` 会复制整个地址空间——实际用 COW（Copy-On-Write），只复制页表，物理页共享直到写操作
2. 混淆 `clone()` 的 flag 组合——`CLONE_VM` 共享内存，`CLONE_FILES` 共享文件描述符表，`CLONE_THREAD` 加入同一线程组
3. 以为内核线程和用户线程的创建方式相同——内核线程用 `kthread_create()`/`kthread_run()`，不走 `clone()` 系统调用

---

<details>
<summary>自测题（点击展开）</summary>

**Q1.** `fork()` 后子进程的 `task_struct` 哪些字段会变？哪些不变？

<details><summary>答案</summary>

变：PID、PPID（=父 PID）、信号_pending 清空、`mm` 的引用计数+1、页表 COW 复制。不变：`mm` 指针（共享但 COW）、`files`（共享但引用计数+1）、`fs`（共享 CWD/root）、调度策略/nice。`fork()` 返回值：父进程=子 PID，子进程=0。

</details>

**Q2.** `clone(CLONE_VM | CLONE_FILES | CLONE_SIGHAND | CLONE_THREAD)` 创建的是什么？

<details><summary>答案</summary>

线程。`CLONE_VM` 共享地址空间，`CLONE_FILES` 共享 fd 表，`CLONE_SIGHAND` 共享信号处理，`CLONE_THREAD` 放入同一线程组（`tgid` 相同，`pid` 不同）。这就是 `pthread_create()` 底层的 `clone()` 调用。

</details>

**Q3.** 内核线程为什么 `mm` 为 NULL？它怎么访问内核内存？

<details><summary>答案</summary>

内核线程不拥有用户地址空间，`task_struct->mm = NULL`。它通过 `active_mm`（借用的 `mm_struct`）访问内核态地址（内核地址空间在所有 `mm_struct` 中都相同）。`active_mm` 在 schedule 时被设置为前一个用户进程的 `mm`，避免 TLB 刷新。

</details>

</details>

---

← [1. 本章定位](./section-1-本章定位.md) · 下一节 [3. 进程描述符](./section-3-进程描述符.md)
