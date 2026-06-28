## 3. 进程描述符 (Process Descriptor)

---

### 一、`task_struct`

内核用 **`task_struct`** 跟踪每个进程（LWP）的几乎全部信息：

- 进程状态、优先级  
- 地址空间指针  
- 挂起的信号  
- 打开的文件、文件系统信息  
- 调度相关字段  
- …（庞大而复杂）

读源码时：`include/linux/sched.h`（modern 内核结构有演进，**概念不变**）。

→ 调度字段深潜：[Ch 7](../../chapter-07-process-scheduling.md)

---

### 二、进程状态（互斥）

生命周期中处于以下状态之一：

| 状态 | 含义 |
|------|------|
| `TASK_RUNNING` | 正在 CPU 上跑，或**就绪**等待 CPU |
| `TASK_INTERRUPTIBLE` | **可中断睡眠** — 等事件/信号，可被信号唤醒 |
| `TASK_UNINTERRUPTIBLE` | **不可中断睡眠** — 如等磁盘 I/O，通常不希望被信号打断 |
| `TASK_STOPPED` | 停止（如 SIGSTOP） |
| `TASK_TRACED` | 被调试器追踪 |
| `EXIT_ZOMBIE` | 已终止，父进程尚未 `wait` 回收 |
| `EXIT_DEAD` | 彻底死亡 |

睡眠/唤醒机制 → [section-4 等待队列](./section-4-organization-and-lookup.md)

---

### 三、内核栈与 `thread_info`

为省内存、提效率，Linux 将 **`thread_info`** 与**内核栈**紧凑放在一起：

- 通常占 **2 个页框（8 KB）**（2.6 经典配置）
- 快速访问当前 CPU 上正在运行的 task

Modern x86-64 多用 **`current` 宏** 通过 per-CPU 或寄存器获取 `task_struct`，实现细节随架构变化。

---

← [2. 进程与线程](./section-2-processes-and-threads.md) · 下一节 [4. 组织与查找](./section-4-organization-and-lookup.md)
