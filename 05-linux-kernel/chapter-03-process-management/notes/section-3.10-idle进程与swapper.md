# 3.10 idle 进程（swapper，PID=0）与 idle 调度类

> 接 §3.9 全局任务链表、§2.9 调度类与"一切皆文件"。本节把 **idle 进程**（task_struct 实例）和 **idle 调度类**（`idle_sched_class`）两件事一次讲清——它们名字像但本质不同，面试高频混淆点。

---

## idle 进程（swapper，PID=0）

`idle` 进程，也叫 **swapper 进程**，`pid = 0`，Linux 内核**第一个任务**，静态编译生成，不是 fork 出来。

> "swapper"名字是 BSD 历史遗留——BSD 的 pid 0 真的做内存换页，Linux 借用名字但 idle **不做 swap**，纯粹历史命名。

### 基础属性

1. **PID = 0**，0 号进程，内核线程（`PF_KTHREAD`），全程运行在内核态，**没有用户地址空间**（`mm == NULL`）。
2. 就是前面全局任务链表的表头本体：`init_task`。

```c
struct task_struct init_task;   // idle/swapper，编译期静态分配
```

定义在 `init/init_task.c`（老内核在 `arch/<arch>/kernel/init_task.c`），用 `INIT_TASK` 宏（`include/linux/init_task.h`）静态初始化，不是运行时赋值。全局任务链表头：`&init_task.tasks`。

3. 内核启动最早就存在，**不来自 fork**；1 号进程（systemd/init）和 2 号进程（kthreadd）都由 PID 0 通过 `kernel_thread()` 创建。

### PID 0 的完整生命周期

关键澄清：**PID 0 不是"生下来就只发呆"**。它先干完早期初始化活，才"退休"进入永久 idle。

时序（`init/main.c` 的 `start_kernel` → `rest_init`）：

```c
noinline void __init rest_init(void)
{
    /* PID 0 在此刻还是"干活状态" */
    pid = kernel_thread(kernel_init, NULL, CLONE_FS);   // → PID 1 (systemd)
    pid = kernel_thread(kthreadd,  NULL, CLONE_FS);     // → PID 2 (kthreadd)
    ...
    cpu_startup_entry(CPUHP_AP_ONLINE_DYN);   // ← PID 0 从这里才真正进入 idle 循环
}
```

所以准确表述：**PID 0 先执行 `start_kernel`/`rest_init` 做全部早期初始化 + 派生 PID 1/2，然后才进入永久 idle**。"idle 不做业务工作"只对 `cpu_startup_entry` 之后成立。

### fork 树：繁衍用户空间 + 内核线程群

```
idle(pid0 swapper)
 ├── kernel_thread → init(pid1, systemd)        【用户空间一号进程】
 │                   └── fork/exec → 所有普通用户进程
 └── kernel_thread → kthreadd(pid2)            【内核线程守护进程】
                       └── fork → kworker、ksoftirqd、migration、watchdog...
```

- PID 1 是所有用户进程的祖先，孤儿进程回收方。
- PID 2 是所有内核线程的祖先，`ps -ef | grep kthreadd` 能看到 PPID=2 的内核线程群。
- PID 0 只存在内核态。

### idle 核心作用

#### 1）CPU 空闲任务

每个 CPU 核都有自己的 idle 线程。CPU 没有就绪可运行任务时，调度器切到 idle 线程。

现代调用链（`kernel/sched/idle.c`）：

```c
cpu_startup_entry(cpu)       // 入口，按 CPU 状态机推进
  → do_idle()                // idle 主循环
    → cpuidle_idle_loop()    // 选 C-state、进省电
```

> 老内核的 `cpu_idle()` 已被重命名，现代代码里说 `do_idle` 更准。

`do_idle()` 循环里检查 `need_resched()`，没有就绪任务就一直循环。

idle **不跑用户业务代码**，但会在循环间隙推进内核的 deferred work（RCU callbacks、irq work 等）。所以"纯粹什么都不做"作为口语理解 OK，精确说法是：idle 不跑用户业务，但会推进被推迟的内核工作。

> 机器负载低时大量时间跑 idle；top 的 `%id` 就是 CPU 在 idle 循环里的 ticks 累计。注意记账是 **per-CPU** 的（`account_idle_time()` 写到 `kstat_cpu->cpustat[CPUTIME_IDLE]`），不是读 idle task 自己的 `stime`。

#### 2）全局任务链表表头

```c
&init_task.tasks   // 全局进程链表头
```

系统所有进程组长都挂在这个双向循环链表上。`ps` 经 procfs 后端 `proc_pid_readdir` → `for_each_process` 遍历这条链表（中间隔了 procfs 一层，不是直接读）。

#### 3）调度类：idle_sched_class（最低优先级）

idle 任务的 `task_struct->sched_class = &idle_sched_class`，是整个调度类层级里**优先级最低**的一档。详见下文。

### SMP：per-CPU idle

- 早期内核：全局只有 1 个 pid=0。
- 现代 SMP：每个 CPU 核有独立 idle 线程，由 `fork_idle()` 在 `kernel/smpboot.c` 的 `idle_threads_init()` 里创建。
- **所有 per-CPU idle 的 pid 都是 0**（N 个 CPU = N 个 pid=0 任务，都挂在 `init_task.tasks` 全局链上）。
- `init_task` 是 0 号 CPU 的 idle，也是全局链表哨兵头；非 0 号 CPU 的 idle 会被 `for_each_process` 遍历到，但 pid=0 所以 ps 不显示。

> 接 §3.9：`for_each_process` 从 `init_task` 出发、遇到它就停，**`init_task` 本身不会被输出**，但它创建的 per-CPU idle 会。

### HFT 视角：C-states 与 `idle=poll`

idle 进 `cpuidle_idle_loop` 后选一个 **C-state**（省电等级）：

| C-state | 唤醒延迟 | 省电 |
|---|---|---|
| C0 (poll) | ~0 | 不省电 |
| C1 (HLT) | ~10μs | 轻度 |
| C6 (deep) | 100μs~1ms | 深度省电 |

HFT 机器基本都关掉深 C-state，换低延迟：

- **内核启动参数 `processor.max_cstate=0`**：禁止进 C1 以下，CPU 永远在 C0/C1 poll。
- **`idle=poll`**：彻底绕过 cpuidle，idle 就是忙等循环。

代价是满载功耗和发热，换来的是"包到达 → CPU 醒来 → 处理"路径上几十到几百 μs 抖动消失。做低延迟交易机这是第一步要改的，比绑核还优先。

顺带 NOHZ：CPU 进入 idle 且没有待处理 tick 事件时，NOHZ 会停掉该 CPU 的周期性 tick（1000Hz tick 本身会打断 idle 睡眠、造成抖动）。`/sys/kernel/sched/features` 里的 `NOHZ_IDLE` 开关控制这个（接 §2.9 的 HFT 接口表）。

---

## idle 调度类 `idle_sched_class`

内核专门给 **每个 CPU 的 idle 线程**用的调度类，是整个调度类层级里**优先级最低**的一档。

### 源码定义

```c
// kernel/sched/idle.c
const struct sched_class idle_sched_class = {
    .enqueue_task        = enqueue_task_idle,
    .dequeue_task        = dequeue_task_idle,
    .yield_task          = yield_task_idle,
    .check_preempt_curr  = check_preempt_curr_idle,
    .pick_next_task      = pick_next_task_idle,
    .put_prev_task       = put_prev_task_idle,
    // ...
};
```

> ⚠️勘误：网上和老资料里常见 `.next = &stop_sched_class`——**这是错的**。`idle_sched_class` 是最低优先级，`.next` 在老内核（用 `.next` 串链的年代）指向的是 NULL 或更高一级的 `fair_sched_class`，绝不会指向最高的 `stop`。而且**现代内核（5.x+）已经没有 `.next` 字段了**，sched_class 通过 linker section（`__sched_classes_section`）+ `for_each_class` 宏按地址顺序遍历，优先级顺序在链接期固定。

### 调度类优先级顺序（从高到低）

```
stop_sched_class  >  dl_sched_class  >  rt_sched_class  >  fair_sched_class  >  idle_sched_class
   最高(停机迁移)    (DEADLINE截止)     (FIFO/RR实时)      (CFS普通)             最低(空闲)
```

> 完整 5 档，别漏 `dl_sched_class`（SCHED_DEADLINE，2.6.23 后引入）。`idle` 是最低，只要前 4 档里有任何可运行任务，永远不会选 idle。

### idle 调度类行为特点

- **idle 线程不进 CFS 红黑树**：不走 `enqueue_task_fair`，`enqueue_task_idle` 基本是空操作。但每个 CPU 的 `rq` 里有一个 `rq->idle` 指针指向本 CPU 的 idle task——idle task 是"常驻 rq"的，不参与排队但被 rq 直接持有。
- **`pick_next_task_idle`**：当 rq 无可运行任务，无条件返回 `rq->idle`。
- **idle 不能抢占别的任务，只有别的任务抢占 idle**：一旦有新任务就绪（`check_preempt_curr` 被触发），立刻把 idle 赶下去。`check_preempt_curr_idle` 实质上总是标记需要 resched。
- **`yield_task_idle` 是空操作**：idle 本来就是最低优先级，yield 没意义。
- **idle 线程没有时间片概念**：不参与 CFS 的 vruntime 计算，也不被 tick 系统调度切换。

### ⚠️ 高频混淆：`SCHED_IDLE` 策略 ≠ `idle_sched_class`

这是面试最爱挖的坑：

| | `SCHED_IDLE`（策略） | `idle_sched_class`（调度类） |
|---|---|---|
| 本质 | **调度策略**（`policy` 字段的一个取值） | **调度类**（`sched_class` 指针指向的对象） |
| 归属 | 仍然属于 `fair_sched_class`（CFS） | 独立的第 5 档调度类 |
| 适用对象 | **用户进程**（`SCHED_IDLE` 设给自己写的程序） | **per-CPU idle 线程**（内核创建，pid=0） |
| 是否进 CFS 红黑树 | ✅ 进，但权重极小（`sched_prio_to_weight[0]`，约 15） | ❌ 不进 |
| 能否跑用户态代码 | ✅ 是普通用户进程 | ❌ 只跑 `do_idle()` 循环 |
| 优先级 | CFS 内最低权重，但**仍高于 idle 线程** | 整个系统最低 |

> 关键区别：`SCHED_IDLE` 进程**仍然会抢占 idle 线程**——它属于 `fair_sched_class`，优先级高于 `idle_sched_class`。所以一台机器即使跑满了 `SCHED_IDLE` 的 nice 19 进程，CPU 也不会进 C-state 睡眠。
>
> 设 `SCHED_IDLE` 的命令：`chrt -i 0 ./my_low_prio_task`，底层走 `sched_setscheduler(SCHED_IDLE)`。

### 和全局任务链表的区分

- `idle_sched_class`：**调度器层面**。控制 idle 线程怎么被调度、什么时候跑，属于调度子系统。
- `init_task.tasks`：**全局双向链表头**。用来遍历系统所有进程组长，和调度没关系，是进程枚举用的。

> 一句话：**调度类管"什么时候跑"，tasks 链表管"能被遍历到"**。idle 线程两者都沾，但两套机制完全独立。

---

## 容易混淆对比

| 进程 | PID | 来源 | 作用 |
|---|---|---|---|
| idle(swapper) | 0 | 编译静态生成（`INIT_TASK` 宏） | CPU 空闲任务、全局任务链表哨兵头、调度类 `idle_sched_class` 最低优先级 |
| init/systemd | 1 | PID 0 通过 `kernel_thread` 创建 | 用户空间第一个进程，孤儿进程回收，启动系统服务 |
| kthreadd | 2 | PID 0 通过 `kernel_thread` 创建 | 内核线程守护进程，所有内核线程的祖先 |

## 面试高频坑

1. ❌误区：idle 是 fork 出来的进程
   ✅：`init_task` 是静态全局结构体，编译就存在，没有经过 fork。

2. ❌误区：机器只有一个 idle 进程
   ✅：多核 SMP，每个 CPU 核心拥有专属 idle 线程，每个核没事就跑自己的 idle。**所有 per-CPU idle 的 pid 都是 0**。top 按 `1` 看各 CPU，每个 CPU 空闲时间就是各自 idle 线程消耗。

3. ❌误区：idle 会出现在用户态
   ✅：idle 全程运行在内核态，没有用户地址空间（`mm == NULL`）。

4. ❌误区：idle 从一开始就只发呆
   ✅：PID 0 先执行 `start_kernel`/`rest_init` 做早期初始化 + 派生 PID 1/2，然后才进入永久 idle 循环。

5. ❌误区：CPU 满载时 idle 还会被调度
   ✅：只有运行队列没有可运行任务，调度器才选 idle；业务任务就绪时永远优先跑业务任务（`idle_sched_class` 最低优先级）。

6. ❌误区：`.next = &stop_sched_class`（idle 指向 stop）
   ✅：idle 是最低优先级，不会指向最高的 stop；现代内核已移除 `.next` 字段，改用 linker section 按地址顺序遍历。

7. ❌误区：`SCHED_IDLE` 策略等价 `idle_sched_class`
   ✅：`SCHED_IDLE` 属于 CFS（`fair_sched_class`），只是权重最小；`idle_sched_class` 只属于 pid=0 idle 内核线程。`SCHED_IDLE` 进程仍会抢占 idle 线程。

## Quiz

**Q1：当 CPU 完全满载，100% 占用，idle 进程还会被调度运行吗？**
A：不会。只有当运行队列没有可运行任务，调度器才选择 idle；业务任务就绪时永远优先跑业务任务。机制上：`idle_sched_class` 优先级最低，别的类总有人排队，轮不到 idle 的 `pick_next_task`。

**Q2：一个进程被设成 `SCHED_IDLE` 策略，它和 idle 线程谁先跑？**
A：`SCHED_IDLE` 进程先跑。它仍然属于 `fair_sched_class`，优先级高于 `idle_sched_class`。`SCHED_IDLE` 只是 CFS 里权重最小，但只要它就绪，调度器遍历到 `fair_sched_class` 时就会返回它，轮不到 `idle_sched_class`。

**Q3：`.next = &stop_sched_class` 这行代码对吗？**
A：不对。`idle_sched_class` 是最低优先级，`.next` 不会指向最高的 `stop`。而且现代内核（5.x+）已移除 `.next` 字段，改用 linker section 按地址顺序遍历。

**Q4：链表上有几个 pid=0？**
A：N 个（N=CPU 核数）。`init_task` 是 0 号 CPU 的 idle 哨兵头；非 0 号 CPU 的 idle 由 `fork_idle()` 创建，pid 也是 0，都挂在 `init_task.tasks` 上。`for_each_process` 会遍历到非 0 号 CPU 的 idle，但 pid=0 所以 ps 不显示。

## 关联记忆

- `init_task`(idle) 本身不参与业务节点遍历，它是双向循环链表的哨兵表头。`for_each_process(p)` 从 `init_task` 出发、遇到它就停，不会把 `init_task` 本身取出来（接 §3.9）。
- 但 per-CPU idle（非 0 号 CPU 的）会挂在 `init_task.tasks` 上、会被 `for_each_process` 遍历到，只是 pid=0 所以 ps 不显示。
- idle 用 `idle_sched_class`，是 `sched_class` 层级的最低优先级（接 §2.9）。
- 调度类总览：`stop > dl > rt > fair > idle`，5 档，`dl` 别漏。
