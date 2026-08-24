## ② 进程描述符与任务结构 · task_struct

内核用 **任务列表（task list）** — **环形双向链表** — 串联所有进程的 `task_struct`；信号批量处理、`ps`/`top` 遍历都依赖这一全局视图。**注意：调度器不依赖它**（见下文「重要误区」）。

### 任务列表（task list）的结构

每个 `task_struct` 内嵌一个链表节点，把全系统任务串成双向循环链表：

```c
struct task_struct {
    // ...
    struct list_head tasks;   // 挂入全局任务链表的链表节点
    // ...
};
```

| 要点 | 说明 |
|------|------|
| 链表头 | **`init_task`** — 0 号进程（pid=0，idle/swapper），静态分配 |
| 节点含义 | 每个节点代表一个任务；**现代内核只挂线程组组长**（见下） |
| 串联方式 | `list_add_tail_rcu(&p->tasks, &init_task.tasks)` 风格的双向循环 |

> **现代内核细节：** `kernel/fork.c` 的 `copy_process()` 中，`list_add_tail_rcu(&p->tasks, &init_task.tasks)` **只在 `thread_group_leader(p)` 为真时执行**——全局任务链表只挂线程组组长（= 用户视角的"进程"）。线程不进这条链，挂到 `signal->thread_head`（经 `thread_node`）和组长的 `thread_group` 链上。老内核（NPTL 初期）曾把所有线程都挂全局链，后来为减小遍历开销改为只挂组长。所以 `for_each_process` 迭代的是"进程"（组长），要摸到线程必须 `for_each_thread(p, t)`，组合宏是 `for_each_process_thread(p, t)`。

### 遍历全部任务（经典写法）

```c
struct task_struct *p;
for_each_process(p) {
    // p 就是每一个进程 task_struct
}
```

`for_each_process` 本质就是遍历这条全局任务双向链表（`include/linux/sched/signal.h`）。线程级遍历用 `for_each_thread(p, t)`。

> 现代内核（5.x/6.x）已不靠遍历任务链表做常规调度，但链表依然存在，主要用于**遍历全部进程**——`ps`、`top`、`kill -9 -1` 批量发信号、OOM 扫描选victim，走的都是它。

### 重要误区：调度器不扫描任务列表 ❌

**CFS 调度器用的是红黑树（`rq->cfs_rq`），不是任务链表。**

- 调度选下一个任务，**不会**遍历整条链表——那是 O(N)，性能不可接受
- 任务链表只承担"全系统进程花名册"角色：`ps`/`top`、遍历所有进程、信号批量处理
- 早期老内核（2.x）曾用全局链表做调度遍历，现代内核早已抛弃

### 任务列表 vs 运行队列

| | **任务列表 (task list)** | **运行队列 (runqueue)** |
|---|---|---|
| 结构 | 双向循环链表 | 红黑树（CFS）/ EEVDF（6.6+） |
| 内容 | **全部**进程（组长级，含休眠的） | 只放**可被调度运行**的任务（线程级） |
| 用途 | 遍历、`ps`、批量信号 | 选下一个上 CPU 的任务 |
| 休眠进程 | **还在**任务链表 | **已退出** runqueue |

> 所以 `for_each_process` 能看到休眠进程，但调度器运行队列里没有它。花名册 vs 干活排队队伍，两回事。

### 父子进程怎么区分？——task_struct 里的另一组链表

全局任务链表**不承担**父子关系。父子关系由 `task_struct` 内的专用字段维护：

```c
struct task_struct {
    // ...
    struct task_struct __rcu *real_parent;  /* 真正的创建者 */
    struct task_struct __rcu *parent;       /* ptrace 时的 tracer（通常 == real_parent） */
    struct list_head children;              /* 我的孩子链表的头节点 */
    struct list_head sibling;               /* 我挂在父亲 children 链上的节点 */
    // ...
};
```

| 操作 | 实现 | 复杂度 |
|------|------|--------|
| **子找父** | 直接指针解引用 `p->real_parent` | **O(1)**，不遍历任何链表 |
| **父找子** | `list_for_each_entry(child, &parent->children, sibling)` | O(孩子数) |
| fork 时建立 | `copy_process()`: `p->real_parent = current` 然后 `list_add_tail(&p->sibling, &p->real_parent->children)` | O(1) |

**关键设计——侵入式链表（intrusive list）：** 一个 `task_struct` 内嵌**多个** `list_head`，同时挂在**多条不同的链**上，互不干扰：

```
task_struct {
    tasks        ──挂──► init_task 的全局任务链（花名册）
    sibling      ──挂──► 父亲的 children 链（我是谁的孩子）
    children     ──头──► 自己孩子链（我的孩子有谁）
    thread_node  ──挂──► 组长 signal->thread_head（我是哪个进程的线程）
    tasks 还在调度侧: sched_entity 挂 runqueue 红黑树（rb_node，非链表）
}
```

回答"父进程怎么找到子进程"：`wait()/waitpid()` 就是遍历自己的 `children` 链找僵尸子进程回收；"子进程怎么找到父进程"：`getppid()` 就是 `current->real_parent->pid`，一次指针解引用。**孤儿进程**：父先退出时 `forget_original_parent()` → `find_new_reaper()` 把孩子托孤给 init/systemd（或 `PR_SET_CHILD_SUBREAPER` 标记的 subreaper），`sibling` 从原父的 `children` 链摘下、挂到新父。

### 任务（task）是不是就是进程？

**不是，精确说：task = 线程级；进程 = 线程组。**

| | **任务 task** | **进程 process（内核叫线程组 thread group）** |
|---|---|---|
| 本体 | 一个 `task_struct` | 共享同一 `tgid` 的一组 task |
| 数量关系 | 调度的最小单位 | 一个进程 = 1 个组长 + N-1 个线程 |
| `pid` vs `tgid` | 每个 task 都有独立 `pid` | 组长 `pid == tgid`；`ps` 显示的是 tgid |
| 调度 | **调度器只认 task**（红黑树里挂的是线程） | "进程"从不被整体调度 |

- 无线程程序：1 task = 1 进程，两者重合——所以日常语境混用没毛病
- 一旦 `pthread_create`：一个进程变成 N 个 task，各自有 `task_struct`、各自的调度实体、各自上红黑树
- 内核源码里没有 "process" 这个一等概念；`for_each_process` 这个名字是历史遗留，它遍历的其实是线程组组长
- 这也解释了 `/proc/<pid>/task/`：目录下每个编号是一个 task 的 pid（组长也叫它线程 id）

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

→ [§3.3 状态](./section-3.3-进程状态.md) · [Ch 4 调度](../../chapter-04-process-scheduling/notes/section-4.3-Linux-调度算法.md) · [Ch 15 §15.2 mm_struct](../../chapter-15-process-address-space/notes/section-15.2-内存描述符.md) · [07 TLPI Ch6 进程环境](../../../03-linux-userspace-api/chapter-06-processes/notes)



<details>
<summary>自测题（点击展开）</summary>

**Q1.** task_struct 存放在哪里？为什么 x86_64 把它放在线程私有区域？

<details><summary>答案</summary>

task_struct 通过 `current` 宏访问。x86_64 把 task_struct 的地址存在 GS 段寄存器指向的 per-CPU 变量中，这样 `current` 只需读 GS:offset，无需遍历链表。这是为了性能：内核代码频繁访问 current（调度/信号/权限检查），O(1) 访问至关重要。

</details>

**Q2.** task_struct 中哪些字段对 HFT 调度优化最关键？

<details><summary>答案</summary>

state（进程状态，决定可否调度）、prio/normal_prio（动态优先级，CFS 用 vruntime 计算）、policy（SCHED_FIFO/SCHED_NORMAL，HFT 用 SCHED_FIFO 绑核）、cpus_allowed（CPU 亲和性，绑核避免 cache miss）、nr_cpus_allowed（可运行 CPU 数）。

</details>

**Q3.** 如果一个进程 sleep 休眠，它还在全局任务链表吗？还在 CFS 红黑树里吗？

<details><summary>答案</summary>

**还在任务链表，不在红黑树。** sleep 时内核把 `task_struct->state` 置为 `TASK_INTERRUPTIBLE`/`TASK_UNINTERRUPTIBLE`，并调用 `dequeue_task()` 把调度实体从运行队列的红黑树里摘出；但 `tasks` 链表节点不动——它只在进程退出（`exit`）时才被摘除。所以 `for_each_process` 依然遍历得到它（`ps` 能看到 sleeping 进程），而调度器在红黑树里找不到它。唤醒（`try_to_wake_up`）时再 `enqueue_task` 放回红黑树。

</details>

</details>
---
