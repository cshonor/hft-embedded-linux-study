# 实时调度类 — FIFO / RR / DEADLINE 与 CFS 的边界

> **定位:** `06-cfs-runtime-mechanics.md` 讲 CFS 内部怎么转，本篇讲**CFS 之外**的实时调度类：FIFO / RR / DEADLINE，以及实时类和 CFS 的优先级关系。
> **关键结论:** 实时调度不是 CFS 的一部分，是**完全独立的调度类**，不走 CFS 的红黑树。

---

## 一、不是一个框架里的"两套调度器"，是多个调度类插件

内核里只有**一个调度框架**（scheduler framework），框架下注册了**多个调度类**（sched_class），每个类自带一整套数据结构和操作函数，内核按优先级从高到低依次挑选任务。

| 调度类 | 管谁 | 机制 |
|------|------|------|
| **dl_sched_class** | SCHED_DEADLINE | EDF 红黑树（按 deadline 排） |
| **rt_sched_class** | SCHED_FIFO / SCHED_RR | 优先级位图数组 |
| **fair_sched_class** | SCHED_OTHER（普通进程，绝大多数程序） | vruntime 红黑树公平分片 |
| **idle_sched_class** | 每 CPU 的 idle 任务 | 系统空闲才跑 |

`struct rq` 里就能看出来——`cfs_rq cfs`、`rt_rq rt`、`dl_rq dl` 是**并列的独立队列**，各有各的数据结构。实时任务**不在** CFS 的红黑树里。

### 调度框架 / 调度类 / 调度策略 三层区分（易混）

| 层 | 是什么 | 例子 |
|----|--------|------|
| **调度框架** | 内核选进程的统一入口和骨架，定义"按类优先级逐层挑选"的规则 | `kernel/sched/core.c` 的 `pick_next_task()` / `__schedule()` |
| **调度类** | 框架下注册的"插件"，每个类一套数据结构 + 入队/出队/挑任务的函数（`sched_class` 结构体里的函数指针） | `fair_sched_class`、`rt_sched_class`、`dl_sched_class`、`idle_sched_class` |
| **调度策略** | 用户态通过 `sched_setscheduler()` 选的**参数**，决定进程进哪个类、类内怎么排队 | `SCHED_OTHER`、`SCHED_FIFO`、`SCHED_RR`、`SCHED_DEADLINE` |

三者关系：**策略决定你进哪个类，类决定用什么算法，框架决定先问哪个类。** 一个类可以对应多个策略（rt_sched_class 同时服务 FIFO 和 RR），反过来一个策略只属于一个类。

---

## 二、三种实时策略

### 1. SCHED_FIFO（POSIX 传统，2.6 至今）

- **无时间片**。高优先级实时进程一旦就绪，直接抢占 CPU
- 只要它不主动让出（sleep / 阻塞），就一直跑；同优先级排队**先进先出**
- 优先级范围 **1-99**（数字越大越高）

**风险与修正:** "FIFO 死循环会把 CPU 占死"——现代内核不成立，有 **RT throttling** 兜底：

```bash
cat /proc/sys/kernel/sched_rt_runtime_us   # 默认 950000
cat /proc/sys/kernel/sched_rt_period_us    # 默认 1000000
```

每个 1s 周期内实时任务**最多跑 950ms**，留 50ms 给 CFS 和内核线程——FIFO 死循环时 shell 还能敲命令救火。只有手动 `echo -1 > sched_rt_runtime_us` 才会真锁死机器。排查实时系统时这两个文件是第一眼要看的。

> 注意：这个限额限的是**整个 RT 类的总量**，同时保护了 DL 任务不被 FIFO 饿死。

### 2. SCHED_RR（POSIX 传统）

- 同样是实时优先级，会抢占 CFS
- **同优先级**之间轮转，时间片默认 100ms（`RR_TIMESLICE`）；耗尽排到同优先级队尾
- **不同优先级**：高优先级依旧直接抢占低优先级

### 3. SCHED_DEADLINE（Linux 3.14 引入，现代实时）

工业、低延迟项目（含 DPDK 场景）大量使用。**是新增，不是替代**——FIFO/RR 仍在用。

**三参数模型（修正：不只 runtime/period 两个）：**

```
runtime / deadline / period
```

例：`runtime=2ms, deadline=4ms, period=10ms` = 每 10ms 一个周期，必须在周期开始后 4ms 内拿到并跑完 2ms。

- 内核实现 = **EDF（最早截止时间优先）**+ **CBS（恒定带宽服务器）**
- "内核会限流"的准确机制：**admission control**——`sched_setattr()` 时就拒绝带宽超标的申请（总带宽 Σ runtime/period ≤ 1），而不是事后限流
- 比 FIFO/RR 安全：带宽被 CBS 硬性框住，不会饿死系统

---

## 三、调度类优先级关系（完整五层）

```text
stop_sched_class   ← 内核内部（migration/热插拔），用户拿不到
dl_sched_class     ← SCHED_DEADLINE (EDF)
rt_sched_class     ← FIFO / RR，优先级 1-99
fair_sched_class   ← CFS / EEVDF
idle_sched_class   ← 每 CPU 的 idle 任务
```

`kernel/sched/core.c` 的 `pick_next_task()` **按这个顺序逐层问"你有活吗"**——只要有实时类任务就绪，CFS 根本轮不到被询问。

---

## 四、权限

- 普通用户默认只能跑 CFS
- root（`CAP_SYS_NICE`）可用 FIFO/RR/DEADLINE
- 非 root 有 `RLIMIT_RTPRIO` 限额时也可申请有限实时优先级

```bash
chrt -f 50 ./task    # SCHED_FIFO 优先级 50
chrt -r 30 ./task    # SCHED_RR
chrt -p <pid>        # 查询某进程的调度策略
```

---

## 五、容易混淆点

| 混淆 | 澄清 |
|------|------|
| Linux 是"两套独立调度器程序"？ | 不是。**一个调度框架 + 多个调度类插件**，框架按类优先级逐层挑选 |
| 实时任务走 CFS 红黑树？ | 不走。CFS 只管 fair 类，RT 用优先级位图，DL 按 deadline 排树 |
| FIFO/RR 被废弃了？ | 没有，POSIX 标准策略一直保留；DEADLINE 是新增第三套 |
| nice 调高能赢实时进程？ | 不能。nice 只在 CFS 类内折算 vruntime 权重，**跨调度类根本不比较** |

---

## 六、对比表

| 策略 | 特点 | 适用场景 |
|------|------|----------|
| SCHED_OTHER (CFS) | 公平分片，nice 调权重 | 绝大多数普通应用、shell、编译器 |
| SCHED_FIFO | 实时，无时间片，抢到就跑到让出 | 对延迟极高、自己可控不会死循环 |
| SCHED_RR | 实时，同优先级轮转时间片 | 同优先级多个同等紧急的实时进程 |
| SCHED_DEADLINE | 截止时间模型，CBS 带宽限制 | 工业、音视频、低延迟业务，更安全的实时 |

---

## Quiz

**Q: 系统同时存在 SCHED_FIFO(优先级 50) 实时进程和 CFS 进程，CFS 的 nice 调到 -20（普通进程最高），能抢过 FIFO 吗？**

<details>
<summary>答案（点开）</summary>

**抢不过，差得远。**

`nice` 是 CFS **类内部**的权重参数——只影响 vruntime 折算率（`NICE_0_LOAD / weight`），决定"CFS 这群人里谁多分点 CPU"。

而 `pick_next_task()` 是**跨类选择，先于一切类内比较**：stop → dl → rt → fair 逐层询问，FIFO(50) 只要就绪，**根本轮不到 fair_sched_class 被问到**，nice 是 -20 还是 +19 一个字都没被读到。

一句话：**nice 调的是类内座位，跨类靠的是出生。** 想赢过实时进程，只有把自己变成实时进程，或让 FIFO 睡觉/退出。
</details>
