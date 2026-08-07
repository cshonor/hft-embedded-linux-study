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

睡眠/唤醒机制 → [section-4 等待队列](./section-4-组织与查找.md)

---

### 三、内核栈与 `thread_info`

为省内存、提效率，Linux 将 **`thread_info`** 与**内核栈**紧凑放在一起：

- 通常占 **2 个页框（8 KB）**（2.6 经典配置）
- 快速访问当前 CPU 上正在运行的 task

Modern x86-64 多用 **`current` 宏** 通过 per-CPU 或寄存器获取 `task_struct`，实现细节随架构变化。

### 常见陷阱

1. 把 ULK 的 `task_struct` 字段布局当现代版——6.x 新增了大量字段（cgroup、seccomp、io_uring、KVM 等），旧字段也有移除
2. 以为 PID 就是 `task_struct->pid`——线程的 `pid` 是线程 ID，`tgid`（线程组 ID）才是用户态看到的「进程 PID」
3. 混淆 `task_struct` 的双向链表和运行队列——`tasks` 链表遍历所有进程，运行队列是 `cfs_rq`/`rt_rq`

---

<details>
<summary>自测题（点击展开）</summary>

**Q1.** 用户态 `getpid()` 返回的是 `task_struct->pid` 还是 `->tgid`？

<details><summary>答案</summary>

返回 `tgid`。内核中 `task_struct->pid` 是唯一的线程 ID，`->tgid` 是线程组 leader 的 PID。用户态的 `getpid()` 实际调 `sys_getpid()` 返回 `current->tgid`。`gettid()` 才返回 `current->pid`。单线程进程 leader 中 `pid == tgid`。

</details>

**Q2.** `task_struct` 中 `tasks` 链表和 `children` 链表有什么区别？

<details><summary>答案</summary>

`tasks`：全局所有 `task_struct` 的双向链表（`init_task` 为头），用于遍历系统所有进程。`children`：当前进程的子进程链表（`init_task` 的 children 是所有孤儿进程的祖先链）。`/proc/[pid]/task/` 遍历的是同一线程组的 `thread_node` 链表。

</details>

**Q3.** 为什么 `task_struct` 不能放在内核栈底部？

<details><summary>答案</summary>

历史原因 + 安全。ULK 时代 x86-32 用 `current` = `ESP & ~8191`（栈底即 `task_struct`），快速但浪费——每个进程 8KB 栈中 2KB 给 `thread_info`。x86-64 改用 per-CPU 变量存 `current`，`thread_info` 移到 `task_struct` 内部。好处是栈空间增大（16KB/32KB），坏处是 `current` 访问多一次 per-CPU 偏移。

</details>

</details>

---

← [2. 进程与线程](./section-2-进程与线程.md) · 下一节 [4. 组织与查找](./section-4-组织与查找.md)
