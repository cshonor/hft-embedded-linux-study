## ② 进程描述符与任务结构 · task_struct

内核用 **任务队列（task list）** — **环形双向链表** — 串联所有进程；调度器、信号、模块遍历都依赖这一全局视图。

| 结构 | 说明 |
|------|------|
| **`task_struct`** | **进程描述符** — 管理该进程所需的 **全部信息**（书中约 **~1.7KB**，随内核版本增长） |
| 链表项 | 每个进程一个 `task_struct` 节点 |
| **`current`** | 宏，指向 **当前 CPU 上** 正在运行的 `task_struct` |

#### task_struct 关键字段（节选）

| 字段 / 子结构 | 职责 |
|---------------|------|
| **`state`** | 运行 / 睡眠 / 停止等 — 见 [§3.3](./section-3.3-进程状态.md) |
| **`pid` / `tgid`** | 线程 ID / 线程组 ID（POSIX 语义） |
| **`mm` / `active_mm`** | 用户地址空间；内核线程 `mm == NULL` |
| **`files`** | 打开文件表（`struct files_struct`） |
| **`signal` / `sighand`** | 挂起信号、处理函数 |
| **`sched`** | 调度实体、优先级、CFS 权重 — 见 [Ch 4](../../chapter-04-process-scheduling/) |
| **`cred`** | UID/GID、能力 |
| **`comm`** | 短命令名（`ps` 可见，16 字节级） |

#### 分配与 thread_info

| 设计 | 目的 |
|------|------|
| **Slab 分配器** 动态分配 `task_struct` | 高效、缓存友好 |
| **`thread_info`** 放在 **进程内核栈底/顶**（架构相关） | 省寄存器、快速定位当前任务 |
| `thread_info` 内指针 | 指向对应 **`task_struct`** |

```
内核栈（4K/8K，架构相关）
┌──────────────────┐
│   栈帧 / 局部变量   │
├──────────────────┤
│  thread_info     │ ──► task_struct（Slab 池）
└──────────────────┘
         ▲
    SP 接近此处时，current 解析极快
```

#### 进程描述符的引用计数

| 机制 | 场景 |
|------|------|
| **`get_task_struct()` / `put_task_struct()`** | 延迟释放，防 use-after-free |
| 退出后僵尸态 | 描述符仍在，等父 `wait` — [§3.6](./section-3.6-进程终结.md) |

#### 与用户态 /proc 对应

| /proc 路径 | 内核侧 |
|------------|--------|
| `/proc/<pid>/status` | state、VmSize、Threads |
| `/proc/<pid>/maps` | `mm` → VMA 列表 — [Ch 15 §15.3](../../chapter-15-process-address-space/notes/section-15.3-虚拟内存区域.md) |
| `/proc/<pid>/task/` | 同 `tgid` 下各线程 |

```c
/* 内核中常见模式（示意） */
struct task_struct *p = current;
if (p->state == TASK_RUNNING)
    /* ... */;
```

**HFT：** 绑核、RT 策略改的是 **`sched` 子结构**；`perf top` 里的 comm 来自 `task_struct->comm`。排查「 mystery 线程」时 `/proc/<pid>/task/*/comm` 与内核栈 trace 要对上同一 `task_struct`。

→ [§3.3 状态](./section-3.3-进程状态.md) · [Ch 4 调度](../../chapter-04-process-scheduling/notes/section-4.3-Linux-调度算法.md) · [Ch 15 §15.2 mm_struct](../../chapter-15-process-address-space/notes/section-15.2-内存描述符.md) · [07 TLPI Ch6 进程环境](../../../../03-linux-userspace-api/chapter-06-processes/notes.md)

---
