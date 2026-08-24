# 任务列表 vs 运行队列 — 花名册不等于排队队伍

> **对标旧书:** LKD3 Ch3 §3.1（进程描述符与任务结构）
> **内核版本:** 任务列表机制 2.0 至今未变；调度侧 CFS (2.6.23) → EEVDF (6.6)
> **关联笔记:** [02-cfs-history.md](./02-cfs-history.md) · [01-eevdf-scheduler.md](./01-eevdf-scheduler.md)

---

## 核心概念

内核里**任务列表（task list）**就是双向循环链表，串联全系统的任务；链表头是 `init_task`。**现代内核只挂线程组组长**（`copy_process()` 里 `list_add_tail_rcu(&p->tasks, &init_task.tasks)` 仅在 `thread_group_leader(p)` 为真时执行），线程挂 `signal->thread_head`，不进这条链。

```c
struct task_struct {
    // ...
    struct list_head tasks;   // 挂入全局任务链表的链表节点
    // ...
};
```

| 要点 | 说明 |
|------|------|
| 链表头 | **`init_task`** — 0 号进程（pid=0，静态分配） |
| 串联方式 | 所有组长进程通过 `tasks` 串成双向循环链表 |
| 经典遍历 | `for_each_process(p)` — 本质就是遍历这条链（迭代组长） |

```c
struct task_struct *p;
for_each_process(p) {
    // p 就是每一个进程 task_struct
}
```

> 现代内核（5.x/6.x）已经**不再靠遍历全局任务链表做常规调度**，但链表依然存在，主要用于遍历全部进程：`ps`、`top`、信号批量处理（`kill -9 -1`）、OOM 扫描选 victim。

---

## 重要误区：调度器不扫描任务列表 ❌

**CFS/EEVDF 调度器用的是红黑树（`rq->cfs_rq`），不是任务链表。**

- 调度选下一个运行任务，**不会**去遍历整条链表——O(N) 性能很差
- 任务链表只是拿来：`ps`、`top`、遍历所有进程、信号批量处理

## 两个概念区分

| | **任务列表 (task list)** | **运行队列 (runqueue)** |
|---|---|---|
| 结构 | 双向循环链表 | 红黑树（CFS）/ EEVDF（6.6+） |
| 内容 | **全部**进程（组长级，含休眠的） | 只放**可被调度运行**的任务（线程级） |
| 用途 | 遍历、`ps`、批量信号 | 选下一个上 CPU 的任务 |
| 休眠进程 | **还在**任务链表 | **已退出** runqueue |

> 休眠的进程依然在全局任务链表（`tasks` 节点只有 `exit` 时才摘除），但已通过 `dequeue_task()` 退出运行队列。所以 `for_each_process` 能看到休眠进程，调度器运行队列里没有它。唤醒（`try_to_wake_up`）时再 `enqueue_task()` 放回。

**一句话总结：** 任务列表 = 全系统进程花名册；调度运行队列 = 准备上 CPU 干活的排队队伍。"内核用任务列表"这句话是对的，但不能理解成调度靠它。

---

## 延伸一：父子关系不靠任务链表

任务链表是"花名册"，**不承担**父子关系。父子关系由 `task_struct` 内的专用字段维护：

```c
struct task_struct {
    // ...
    struct task_struct __rcu *real_parent;  /* 创建者 */
    struct task_struct __rcu *parent;       /* ptrace 的 tracer */
    struct list_head children;              /* 我的孩子链表头 */
    struct list_head sibling;               /* 我挂在父亲 children 链上的节点 */
    // ...
};
```

| 方向 | 实现 | 复杂度 |
|------|------|--------|
| 子找父 | `p->real_parent` 一次指针解引用（`getppid()` 即 `current->real_parent->pid`） | O(1) |
| 父找子 | `list_for_each_entry(child, &parent->children, sibling)`；`wait()/waitpid()` 回收僵尸就靠它 | O(孩子数) |
| fork 建立 | `copy_process()`: `p->real_parent = current` + `list_add_tail(&p->sibling, &p->real_parent->children)` | O(1) |
| 父先退出 | `forget_original_parent()` → `find_new_reaper()` 托孤给 init/subreaper，`sibling` 摘链重挂 | — |

**核心设计——侵入式链表：** 一个 `task_struct` 内嵌多个 `list_head`，同时挂在多条互不干扰的链上：

```
tasks        ──► init_task 全局任务链（花名册，只有组长）
sibling      ──► 父亲的 children 链（我是谁的孩子）
children     ──► 自己的孩子链（我的孩子）
thread_node  ──► 组长 signal->thread_head（我是哪个进程的线程）
sched_entity ──► runqueue 红黑树（rb_node，可调度时才在）
```

## 延伸二：task ≠ process

**task = 线程级（一个 task_struct）；process = 线程组（共享 tgid 的一组 task）。**

- 无线程程序：1 task = 1 进程，重合——日常混用没毛病
- `pthread_create` 之后：一个进程 = N 个 task，调度器**只认 task**，红黑树里挂的是线程不是进程
- `for_each_process` 名字是历史遗留：现代内核任务链表只挂组长（= 用户视角的进程），摸线程用 `for_each_thread(p, t)`，组合宏 `for_each_process_thread(p, t)`
- `/proc/<pid>/task/` 下每个编号 = 一个 task 的 pid（组长的 pid == tgid）

---

## 与旧书差异

| LKD3 (2010) 讲的 | 现代内核实际 | 说明 |
|-------------------|-------------|------|
| 全局任务链表描述 | 机制本身未变 | `tasks` + `init_task` + `for_each_process` 沿用至今 |
| 任务链表挂所有 task | **只挂线程组组长** | 线程走 `signal->thread_head`；`for_each_process` 实际迭代"进程" |
| 调度依赖数据结构 | CFS 红黑树 → EEVDF | 任务链表与调度器从来就是两套数据结构 |
| `for_each_process` 定义位置 | `include/linux/sched/signal.h` | 从 `sched.h` 拆分出去的 |

---

## HFT 关联

| 场景 | 说明 |
|------|------|
| **排查线程延迟毛刺** | `ps`/`top` 看到的是任务链表视图（全部任务）；`perf sched`/`runqlat` (BCC) 看到的是运行队列视图（排队延迟）——两套视图要分清 |
| **runqlat 监控** | HFT 低延迟标配：`runqlat` 直方图监控运行队列深度，队列长 = 调度延迟大 |
| **热路径不碰全局链表** | 遍历 `for_each_process` 要拿全局读锁，系统进程多时是慢操作——交易热路径上永远不该出现 |

---

<details>
<summary>自测题（点击展开）</summary>

**Q1:** 如果一个进程 sleep 休眠，它还在全局任务链表吗？还在 CFS 红黑树里吗？

> 还在任务链表，不在红黑树。sleep 时 `task_struct->state` 置为 `TASK_INTERRUPTIBLE`/`TASK_UNINTERRUPTIBLE`，`dequeue_task()` 把调度实体从红黑树摘出；但 `tasks` 链表节点不动，只有 `exit` 时才摘除。所以 `ps` 能看到 sleeping 进程，调度器找不到它。

**Q2:** 为什么调度器不能用 `for_each_process` 遍历选任务？

> O(N)：现代系统动辄上千任务，每次调度都遍历全链表开销不可接受。红黑树按 vruntime 排序，取最左节点 O(log n)；EEVDF 进一步用就近的 deadline 语义。数据结构必须匹配调度决策"找最值"的访问模式。

**Q3:** `for_each_process` 能遍历到线程吗？如果要遍历某个进程的所有线程怎么办？

> 现代内核不能——任务链表只挂线程组组长（`copy_process()` 里 `list_add_tail_rcu(&p->tasks, &init_task.tasks)` 仅在 `thread_group_leader(p)` 为真时执行）。`for_each_process` 迭代的是"进程"（组长）。要遍历某进程的线程用 `for_each_thread(p, t)`，走 `signal->thread_head`（`thread_node` 挂入）；两者组合的宏是 `for_each_process_thread(p, t)`。老内核（NPTL 初期）确实把所有线程挂全局链，后来为减小遍历开销改为只挂组长。

**Q4:** 父进程怎么找到子进程？子进程怎么找到父进程？

> 子找父：`p->real_parent` 一次指针解引用，O(1)。父找子：遍历自己的 `children` 链（`list_for_each_entry(child, &parent->children, sibling)`），`wait()` 回收僵尸就是这条路。fork 时 `copy_process()` 同时建立两边：设 `real_parent` 指针 + 把 `sibling` 挂入父亲的 `children`。这是侵入式链表设计的典型应用——`task_struct` 内嵌多个 `list_head`，同时挂在全局任务链、父亲的 children 链、线程组链上，互不干扰。

**Q5:** 内核语境里"任务"就是"进程"吗？

> 不是。task = 一个 task_struct = 一个调度实体 = 用户视角的一个线程。process = 线程组 = 共享 tgid 的一组 task（组长 pid == tgid）。无线程程序两者重合所以日常混用没问题；一旦创建线程，一个进程就是 N 个 task，调度器只认 task。`for_each_process` 遍历的其实是组长（进程），这个名字是历史遗留。

</details>
