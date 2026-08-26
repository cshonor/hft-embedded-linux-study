# 2.10 调度类 `struct sched_class` 总览

> 接 §2.9（sched_class 与"一切皆文件"）、§3.10（idle 进程与 idle_sched_class）。本节是**调度类层级总览**——5 档优先级、用户/内核边界、`pick_next_task` 遍历机制、`sched_setscheduler` 切换动作。

> 核心一句话：
> **`struct sched_class` 是一组函数指针集合（vtable）；每个线程 `task_struct` 里存一个指针 `sched_class`，指向某一组函数。这是内核内部使用，用户空间完全看不见，用户程序不能直接改写这个结构体**。

```c
struct task_struct {
    // ...
    const struct sched_class *sched_class;
    // ...
};
```

每个任务会绑定某一个调度类实例：
- 普通进程 → `&fair_sched_class`（CFS）
- 实时 FIFO/RR → `&rt_sched_class`
- 截止时间 → `&dl_sched_class`
- idle 线程 → `&idle_sched_class`
- stop 线程 → `&stop_sched_class`

---

## 1、调度类给谁用？给谁调用？

**只给内核调度子系统内部调用，用户态完全不接触。**

用户空间做的，只是调用系统调用：

```c
sched_setscheduler(pid, SCHED_FIFO, &param);
```

用户态系统调用，**仅仅是告诉内核：把这个 task 的 `sched_class` 换成 `rt_sched_class`**。真正的调度逻辑、入队、出队、选下一个任务，全部内核内部通过函数指针调用调度类的成员函数。

内核调度器通用代码不会写死 CFS / RT 逻辑，全部通过函数指针回调。伪代码示意：

```c
// 内核要把一个任务放进运行队列 rq
static void enqueue_task(struct rq *rq, struct task_struct *p)
{
    // 不写死 if 判断 CFS/RT/idle
    // 直接调用这个任务自己绑定调度类的 enqueue_task 函数
    p->sched_class->enqueue_task(rq, p, flags);
}
```

> 如果 p 是 CFS 任务，就执行 `fair_sched_class.enqueue_task`（插入 CFS 红黑树）；
> 如果 p 是实时任务，执行 `rt_sched_class.enqueue_task`（插入 rt 优先级数组）；
> idle 任务不走常规 enqueue。

> ⚠️勘误：选下一个任务**不是** `rq->curr->sched_class->pick_next_task(rq)`（只看当前任务的调度类）——而是**从高到低遍历所有 sched_class**，见第 4 节。

**同一个通用调度框架，依靠不同的 `sched_class` 函数指针，执行完全不同的调度算法。** 这就是内核里面向对象的实现方式：C 语言没有 class，用函数指针模拟多态（和 VFS 的 `file_operations`、`inode_operations`、`vm_operations_struct` 是同一套思路）。

---

## 2、5 个调度类（优先级从高到低）

| 调度类 | 适用对象 | 用户能否设置 | 入队数据结构 |
|---|---|---|---|
| `stop_sched_class` | migration 线程（`migration/N`）、`stop_machine`、CPU hotplug、rt throttle 等内核内部机制 | ❌ 纯内核内部 | 不入队，`rq->stop` 直接持有 |
| `dl_sched_class` | SCHED_DEADLINE 截止时间调度（EDF 算法，2.6.23 后引入） | ✅ `sched_setscheduler(SCHED_DEADLINE)` | 专用 dl 红黑树（按 deadline） |
| `rt_sched_class` | SCHED_FIFO / SCHED_RR 实时进程 | ✅ `sched_setscheduler(SCHED_FIFO/RR)` | rt 优先级数组（位图+队列） |
| `fair_sched_class` | SCHED_OTHER / SCHED_BATCH / **SCHED_IDLE** 普通分时进程 | ✅ 默认或显式设置 | CFS 红黑树（按 vruntime） |
| `idle_sched_class` | per-CPU idle 线程（pid=0） | ❌ 内核创建，用户不能设置 | 不入队，`rq->idle` 直接持有 |

> ⚠️勘误：网上常见只列 4 档（漏 `dl_sched_class`）。完整 5 档，`dl` 在 `rt` 之上——SCHED_DEADLINE 任务优先级比 SCHED_FIFO 还高。
>
> ⚠️勘误：写"SCHED_RT"不规范。实时策略只有 `SCHED_FIFO` 和 `SCHED_RR` 两个取值，没有"SCHED_RT"这个常量。

---

## 3、用户态系统调用和内核调度类之间关系

| 用户态调用 | 效果（内核内部发生） |
|---|---|
| 默认 fork 出来 | 继承父进程 policy；默认 SCHED_OTHER → `sched_class = &fair_sched_class` |
| `sched_setscheduler(..., SCHED_FIFO)` | `sched_class = &rt_sched_class`，从旧队列 dequeue 后按 rt 入队 |
| `sched_setscheduler(..., SCHED_RR)` | 同上，`rt_sched_class`，但同优先级有时间片轮转 |
| `sched_setscheduler(..., SCHED_DEADLINE)` | `sched_class = &dl_sched_class`，需指定 runtime/deadline/period |
| `sched_setscheduler(..., SCHED_IDLE)` | **`sched_class` 仍然是 `&fair_sched_class`**，只是权重固定为最小值 |
| `sched_setscheduler(..., SCHED_BATCH)` | 仍然是 `&fair_sched_class`，批处理风格（CPU 评估型，不抢占） |
| CPU 的 idle 线程 | `sched_class = &idle_sched_class`；用户不能修改 |

**用户空间不能直接操作 `sched_class` 指针。全部由内核根据调度策略间接赋值。**

> ⚠️勘误："SCHED_IDLE 只是降低 nice 权重"会误导。`SCHED_IDLE` 是**独立的 policy**（不是 nice 值），它把进程留在 `fair_sched_class`，但权重固定为 `sched_prio_to_weight[0]` 的最小值（约 15），vruntime 增速最快。和 nice 19 是两套机制——nice 19 仍属于 `SCHED_OTHER`，权重比 `SCHED_IDLE` 高得多。实际效果上 `SCHED_IDLE` 比 nice 19 还弱（详见 §3.10 对比表）。

---

## 4、运行队列 rq 和 `pick_next_task` 遍历机制

每个 CPU 有一个 `struct rq` 运行队列。`rq` 内部包含各个调度类对应的子队列（`rq->cfs`、`rq->rt`、`rq->dl`），以及 `rq->idle`、`rq->stop` 两个直接指针。

调度器选下一个任务用 `pick_next_task`，**从高到低遍历 sched_class**（现代内核用 `for_each_class` 宏，按 linker section 地址顺序遍历）：

```c
struct task_struct *pick_next_task(struct rq *rq, ...)
{
    /* fast path：如果运行队列里全是 CFS 任务，跳过 stop/dl/rt 直接走 CFS */
    if (likely(rq->nr_running == rq->cfs.nr_running))
        return pick_next_task_fair(rq, ...);

    for_each_class(class) {            // stop → dl → rt → fair → idle
        p = class->pick_next_task(rq, ...);
        if (p) return p;
    }
    /* 全部 class 都没拿到任务，返回 rq->idle */
}
```

> 关键点：**不是看当前任务的调度类选下一个，而是遍历所有调度类**。`pick_next_task_idle`（最低档）只在 stop/dl/rt/fair 全部返回 NULL 时才被调用，无条件返回 `rq->idle`。
>
> fast path 是性能优化：大多数机器绝大多数 CPU 都只跑 CFS 任务，跳过 stop/dl/rt 的检查能省不少周期。这个优化在 HFT 场景下也值得知道——它意味着一个 RT 任务被唤醒会强制走 slow path。

---

## 5、`sched_setscheduler` 切换调度类的完整动作

切换不只是改指针，**必须重新入队**——因为不同调度类的队列数据结构完全不同（CFS 红黑树 vs rt 位图 vs dl 红黑树），不重新入队就会丢失。`__setscheduler` 做三步：

```c
1. dequeue_task(rq, p, DEQUEUE_HEAD);   // 从旧调度类的队列里取出
2. p->sched_class = &new_class;          // 改指针
3. enqueue_task(rq, p, ENQUEUE_HEAD);    // 按新调度类的规则重新入队
```

切换后还会 `check_preempt_curr` 看是否需要立即抢占当前任务。

> fork 时走 `sched_fork` → `__sched_fork`，根据父进程 policy 初始化 `sched_class`。如果父进程是 `SCHED_FIFO`，子进程默认也是 `SCHED_FIFO`（除非显式 `sched_setscheduler` 改）。
>
> 同调度类内切换（如 SCHED_OTHER → SCHED_IDLE，都属 `fair_sched_class`）**不需要重新入队**，只改 policy 字段和权重计算路径。

---

## 6、面试易错点总结

1. ❌误区：用户程序可以直接修改 `task_struct->sched_class`
   ✅：用户态拿不到 `task_struct`，只能通过 `sched_setscheduler` 系统调用让内核间接变更，且内核会重新入队。

2. ❌误区：`SCHED_IDLE` 用户策略等价 `idle_sched_class`
   ✅：`SCHED_IDLE` 属于 CFS（`fair_sched_class`），只是权重最小；`idle_sched_class` 只属于 pid=0 idle 内核线程。两者完全两码事。`SCHED_IDLE` 进程仍会抢占 idle 线程。

3. ❌误区：调度类是给用户态使用
   ✅：调度类结构体全部内核态，函数指针全部内核调用，是内核实现"不同调度算法"的多态机制。

4. ❌误区：`SCHED_IDLE` 就是 nice 19
   ✅：`SCHED_IDLE` 是独立 policy，权重固定为最小值；nice 19 是 `SCHED_OTHER` + nice 调整，权重比 `SCHED_IDLE` 高。实际效果上 `SCHED_IDLE` 比 nice 19 还弱。

5. ❌误区：调度类只有 4 档
   ✅：完整 5 档——`stop > dl > rt > fair > idle`，别漏 `dl_sched_class`（SCHED_DEADLINE）。

6. ❌误区：选下一个任务看当前任务的调度类
   ✅：`pick_next_task` 从高到低遍历所有 sched_class，第一个返回非 NULL 的就是下一个任务。当前任务的调度类只影响它自己的入队/出队行为。

7. 区分记忆：
   - **调度策略**（`sched_setscheduler` 传的参数）：用户态可见，`SCHED_OTHER/FIFO/RR/IDLE/BATCH/DEADLINE`
   - **调度类 `sched_class`**：内核内部实现调度算法，用户不可见，5 个实例

---

## Quiz

**Q1：一个 SCHED_FIFO 实时线程，`task_struct` 里 `sched_class` 会指向哪一个？用户直接读写这个指针吗？**
A：指向 `&rt_sched_class`；用户态不能直接读写指针，通过 `sched_setscheduler(SCHED_FIFO)` 让内核帮它设置，内核会 dequeue→改指针→enqueue→check_preempt。

**Q2：把一个进程从 `SCHED_OTHER` 改成 `SCHED_IDLE`，`sched_class` 指针变不变？需要重新入队吗？**
A：指针不变，不需要重新入队。两者都属于 `fair_sched_class`，`sched_class` 一直指向 `&fair_sched_class`，变的只是 policy 字段和权重计算路径。

**Q3：`pick_next_task` 是看当前任务的调度类选下一个吗？**
A：不是。是从高到低遍历所有 5 个 sched_class，每个 class 自己看有没有就绪任务，第一个返回非 NULL 的就是下一个任务。当前任务的调度类只影响当前任务自己的入队/出队行为，不影响选下一个任务的遍历顺序。

**Q4：一个 `SCHED_DEADLINE` 任务和一个 `SCHED_FIFO` 任务同时就绪，谁先跑？**
A：`SCHED_DEADLINE` 先跑。`dl_sched_class` 优先级高于 `rt_sched_class`。

**Q5：为什么 `pick_next_task` 有个 fast path 检查 `rq->nr_running == rq->cfs.nr_running`？**
A：性能优化。大多数机器绝大多数 CPU 都只跑 CFS 任务，跳过 stop/dl/rt 的遍历能省周期。一旦有 RT/DL 任务被唤醒，`rq->nr_running` 会大于 `rq->cfs.nr_running`，自动走 slow path。

---

## 关联记忆

- §2.9：`sched_class` 是纯 vtable（函数指针），可调参数在 `fair.c` 全局变量，经 kernfs 暴露为 sysfs 伪文件。
- §3.10：`idle_sched_class`（最低档）只给 per-CPU idle 线程用，不进 CFS 红黑树；`SCHED_IDLE` 策略 ≠ `idle_sched_class`。
- §3.9：`init_task.tasks` 全局链表管"能被遍历到"，`sched_class` 管"什么时候跑"，两套机制独立。
- 调度类层级：`stop > dl > rt > fair > idle`，5 档，别漏 `dl`。
