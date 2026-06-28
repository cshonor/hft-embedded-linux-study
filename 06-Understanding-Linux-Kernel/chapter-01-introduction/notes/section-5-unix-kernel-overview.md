## 5. Unix 内核概述

> Ch 1 **重头戏** — 各子系统模型概览；全书后续章逐块源码级展开

---

### 一、进程 / 内核模型

进程在以下情况从 **用户态 → 内核态**：

- 硬件中断
- 定时器中断
- **系统调用**

内核代表该进程执行特权操作，并通过 **进程描述符（Process descriptor）** 跟踪进程状态。

→ 深潜：[Ch 3 进程](../../chapter-03-processes.md) · [Ch 4 中断与异常](../../chapter-04-interrupts-and-exceptions.md) · [Ch 10 系统调用](../../chapter-10-system-calls.md)

---

### 二、可重入内核与同步

- **可重入（Reentrant）：** 多个进程可**同时处于内核态**。
- 控制路径可能**交错**（如被中断打断）→ 必须用同步保护 **临界区（Critical Regions）**，避免 **竞态（Race conditions）**。

Ch 1 初步介绍的原语：

| 原语 | 用途 |
|------|------|
| 原子操作 | 不可分割的读-改-写 |
| 信号量（Semaphore） | 睡眠式互斥 / 计数 |
| 自旋锁（Spin lock） | 短临界区、忙等 |

→ 深潜：[Ch 5 内核同步](../../chapter-05-kernel-synchronization.md)

---

### 三、进程间通信与信号

| 机制 | 作用 |
|------|------|
| **信号（Signals）** | 通知进程系统事件（如 SIGINT、SIGCHLD） |
| **IPC** | 进程间数据交换：信号量、消息队列、共享内存等 |

→ 深潜：[Ch 11 信号](../../chapter-11-signals.md) · [Ch 19 进程通信](../../chapter-19-ipc.md)

---

### 四、进程管理

| 调用 / 概念 | 说明 |
|-------------|------|
| `fork()` | 创建子进程；**写时复制（COW）** 共享地址空间 |
| `_exit()` | 终止进程 |
| `exec()` 族 | 加载新程序映像 |
| `init` | 收养孤儿进程 |
| 进程组 / 会话 | 作业控制、终端关联 |

→ 深潜：[Ch 3 进程](../../chapter-03-processes.md) · [Ch 20 程序执行](../../chapter-20-program-execution.md) · [01 CSAPP](../../../01-CSAPP-3rd/) Ch 8

---

### 五、内存管理（概述）

| 概念 | 说明 |
|------|------|
| **虚拟内存** | 每个进程独立地址空间；隔离、安全、可大于物理 RAM |
| **物理内存（RAM）** | 内核分配 **页框（Page frame）** |
| **按需分页（Demand paging）** | 先分配虚拟区域；**真正访问**触发缺页异常时才映射物理页 |

→ 深潜：[Ch 2 内存寻址](../../chapter-02-memory-addressing.md) · [Ch 8 内存管理](../../chapter-08-memory-management.md) · [Ch 9 进程地址空间](../../chapter-09-process-address-space.md) · [07 Gorman](../../../07-Linux-Virtual-Memory-Manager/)

---

### 六、后续章节索引

Ch 1 主题 → ULK 精读章（🔴 = HFT 优先，见 [OUTLINE](../../OUTLINE.md)）：

| Ch 1 主题 | ULK 章节 | 标签 |
|-----------|----------|------|
| 内存寻址、页表 | [Ch 2](../../chapter-02-memory-addressing.md) | 🔴 |
| 进程、进程描述符 | [Ch 3](../../chapter-03-processes.md) | 🔴 |
| 中断、用户态↔内核态 | [Ch 4](../../chapter-04-interrupts-and-exceptions.md) | 🔴 |
| 同步、临界区、SMP | [Ch 5](../../chapter-05-kernel-synchronization.md) | 🔴 |
| 进程调度、抢占 | [Ch 7](../../chapter-07-process-scheduling.md) | 🔴 |
| 物理内存、按需分页 | [Ch 8](../../chapter-08-memory-management.md) · [Ch 9](../../chapter-09-process-address-space.md) | 🔴 |
| 系统调用 | [Ch 10](../../chapter-10-system-calls.md) | 🔴 |
| 信号 | [Ch 11](../../chapter-11-signals.md) | 🟡 |
| VFS、文件访问 | [Ch 12](../../chapter-12-VFS.md) · [Ch 16](../../chapter-16-file-access.md) | ⚪ |
| 模块 | [附录 B](../../appendix-B-modules.md) | 🟡 |
| IPC | [Ch 19](../../chapter-19-ipc.md) | 🟡 |
| exec、程序加载 | [Ch 20](../../chapter-20-program-execution.md) | 🟡 |

---

← [4. Unix 文件系统](./section-4-unix-filesystem-overview.md) · 下一章 [Ch 2 内存寻址](../../chapter-02-memory-addressing.md)
