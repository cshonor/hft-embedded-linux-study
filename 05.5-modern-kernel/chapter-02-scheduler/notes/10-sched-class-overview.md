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

### 关键设计：没有 ID，靠指针地址识别

内核**没有** `int sched_class_id` 这种数字字段来标记"是哪一类调度"。识别调度类就靠**比较指针地址**：

```c
/* 内核里确实有这种直接比较指针的代码 */
if (p->sched_class == &idle_sched_class) {
    /* 当前任务是 idle 线程 */
}
```

指针等于 `&fair_sched_class` 就是 CFS，等于 `&rt_sched_class` 就是实时——不需要 enum、不需要 ID 号。

> **内存效率**：`task_struct` 只存**一个指针**，不携带一整套函数表。成千上万个 task 共享同一组全局 `sched_class` 实例——4 个实例（+ `dl` 共 5 个）编译期固定，全局只读，所有 task 指过去就行。如果每个 task 各存一份函数表，那才是浪费。

### 全局实例的声明

```c
/* 内核全局，只读，编译期固定 */
extern const struct sched_class stop_sched_class;
extern const struct sched_class dl_sched_class;      /* ⚠️ 原笔记漏了这行 */
extern const struct sched_class rt_sched_class;
extern const struct sched_class fair_sched_class;
extern const struct sched_class idle_sched_class;
```

> ⚠️勘误：原笔记只列 4 个 `extern`，漏了 `dl_sched_class`。完整 5 个，见第 2 节。

### 简图

```
全局只读实例（编译就存在）
├─ stop_sched_class  { enqueue(), pick_next(), ... }   最高
├─ dl_sched_class    { enqueue(), pick_next(), ... }
├─ rt_sched_class    { enqueue(), pick_next(), ... }
├─ fair_sched_class  { enqueue(), pick_next(), ... }
└─ idle_sched_class  { enqueue(), pick_next(), ... }   最低

task_struct(进程A，普通)
    sched_class = &fair_sched_class  ── 指向上面 fair 实例

task_struct(进程B，实时)
    sched_class = &rt_sched_class    ── 指向上面 rt 实例

调度器执行 p->sched_class->enqueue_task(rq, p);
顺着指针，调用对应那一套函数。
```

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

### 两层分发机制：指针分大门，policy 分小隔间

上面的映射表揭示一个关键设计——**6 个用户策略只映射到 3 个调度类**（不算 stop/idle）：

| 用户态策略（数字） | 归属哪个内核调度类 |
|---|---|
| `SCHED_FIFO` | `rt_sched_class` |
| `SCHED_RR` | `rt_sched_class`（**和 FIFO 共用同一个**） |
| `SCHED_OTHER` | `fair_sched_class` |
| `SCHED_IDLE` | `fair_sched_class`（**和 OTHER 共用同一个**） |
| `SCHED_BATCH` | `fair_sched_class`（同上） |
| `SCHED_DEADLINE` | `dl_sched_class` |

`SCHED_RR` 和 `SCHED_FIFO` 不是两个独立调度类——只是 `rt_sched_class` **内部的两种行为模式**。同理 `OTHER`/`IDLE`/`BATCH` 是 `fair_sched_class` 内部的三种模式。

> ⚠️勘误：很多资料说"内核 4 个调度类，用户 5 个策略"——两处都错。正确是：**5 个调度类，6 个策略**（漏了 `dl_sched_class` 和 `SCHED_DEADLINE`）。

调度器运行时分两层判断：

```
第一层（大门）：看 p->sched_class 指针，决定进入哪一套函数
第二层（门内）：进入对应调度类函数后，读 p->policy 数字，区分小模式
```

rt 调度类内部伪代码：

```c
/* 属于 rt_sched_class 的函数内部 */
task = 从实时优先级队列取出任务;
if (task->policy == SCHED_FIFO) {
    // FIFO：不消耗时间片，拿到 CPU 一直跑到主动让出
} else if (task->policy == SCHED_RR) {
    // RR：时间片递减，耗尽后放回队列尾部，同优先级轮转
}
```

fair 调度类内部同理，读 `policy` 区分 `SCHED_OTHER`/`SCHED_BATCH`/`SCHED_IDLE` 的权重计算路径。

> **为什么不用 policy 做第一层判断？** 内核设计思想：面向对象，函数指针解耦。第一层不写巨大的 `if (policy == FIFO || policy == RR || ...)`，直接靠指针跳转到整组函数。新增调度算法只要新增一个 `sched_class` 全局实例，不用到处改 if-else。policy 只在调度类内部做细分，不影响外层分发。

> 大白话：**调度类指针负责"进哪个大门"（rt/fair/dl），policy 数字负责"大门里坐哪个小隔间"（FIFO/RR，OTHER/IDLE/BATCH）**。

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

7. ❌误区：内核用数字 ID 标记是什么调度类
   ✅：**直接比较指针地址识别调度类，没有额外 ID 字段**。`p->sched_class == &fair_sched_class` 就是 CFS，不需要 `int sched_class_id`。

8. ❌误区：每个 `task_struct` 存一整套 enqueue/pick_next 函数
   ✅：task 只存**一个指针**，指向全局已写好的函数表。成千上万个 task 共享同一组 `sched_class` 实例，不各自复制。

9. ❌误区：`SCHED_RR` 和 `SCHED_FIFO` 是两个独立调度类
   ✅：两者共用同一个 `rt_sched_class`，只是 rt 调度类内部读 `task->policy` 区分行为。同理 `SCHED_OTHER`/`SCHED_IDLE`/`SCHED_BATCH` 共用 `fair_sched_class`。

10. ❌误区：调度器第一层用 policy 数字做分支判断
   ✅：第一层看 `p->sched_class` 指针，直接跳转到对应函数组。policy 只在调度类**内部**被读取做细分。这是 OOP 解耦设计——新增调度算法加一个 `sched_class` 实例即可，不用到处改 if-else。

11. 区分记忆：
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

**Q6：如果强行把 `task_struct` 的 `sched_class` 指针改成 NULL，内核会怎样？**
A：内核调用 `p->sched_class->enqueue_task` 时对 NULL 指针解引用 → **内核 Oops 崩溃**。所以这个指针绝对不能乱改，全部由内核内部代码维护，用户态无法直接访问 `task_struct`。

**Q7：一个任务设置为 `SCHED_RR`，它的 `sched_class` 指针指向谁？是 policy 决定调用 `enqueue` 吗？**
A：指向 `&rt_sched_class`。policy 不在调度器外层做判断——进入 rt 调度类的函数内部后，才会读 `task->policy` 区分 RR/FIFO。`SCHED_RR` 和 `SCHED_FIFO` 共用同一个 `rt_sched_class`，只是内部行为不同（RR 有时间片轮转，FIFO 没有）。

**Q8：有人说"内核 4 个调度类、用户 5 个策略"，对吗？**
A：不对。正确是**5 个调度类、6 个策略**。漏了 `dl_sched_class` 和 `SCHED_DEADLINE`。`stop > dl > rt > fair > idle` 共 5 档，用户可见策略 6 个（`OTHER/FIFO/RR/IDLE/BATCH/DEADLINE`）。

---

## 关联记忆

- §2.9：`sched_class` 是纯 vtable（函数指针），可调参数在 `fair.c` 全局变量，经 kernfs 暴露为 sysfs 伪文件。
- §3.10：`idle_sched_class`（最低档）只给 per-CPU idle 线程用，不进 CFS 红黑树；`SCHED_IDLE` 策略 ≠ `idle_sched_class`。
- §3.9：`init_task.tasks` 全局链表管"能被遍历到"，`sched_class` 管"什么时候跑"，两套机制独立。
- 调度类层级：`stop > dl > rt > fair > idle`，5 档，别漏 `dl`。
