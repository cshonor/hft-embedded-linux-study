## ⑥ 实时调度策略 · Real-Time Scheduling

RT 任务由 **独立实时调度类** 管理 — **不走 CFS 红黑树**。只要有可运行的 RT 任务，它们就 **压过** 所有普通 CFS 任务。

#### 优先级字段（与 CFS 对照）

| 用户控制 | 内核字段 | 换算到全局 `prio` |
|----------|----------|-------------------|
| 控制参数 | **`rt_priority`（越大越优先）** | `task_struct.rt_priority` | **`normal_prio = prio = 99 - rt_priority`** |
| nice | **无效** | — |

`prio` 越小越优先；RT 落在约 **0…99**，CFS 在 **100…139** → 天然 RT 压过 CFS。  
完整三字段对照 → [§4.2](./section-4.2-调度策略.md)

#### `SCHED_FIFO` vs `SCHED_RR`

| 策略 | 行为 |
|------|------|
| **`SCHED_FIFO`** | 先进先出；**无时间片**；同优先级一直跑到 **阻塞、主动 yield、或被更高 RT 抢占** |
| **`SCHED_RR`** | 同优先级带 **时间片**；用完排回同优先级队尾 |

```
RT 优先级 99 可运行？──是──► 跑它（压过一切更低 RT 与全部 CFS）
        │否
        ▼
同 prio FIFO/RR 队列 ──► 再 ↓ ──► 才轮到 CFS
```

#### 软实时 · Soft Real-time

| 承诺 | 说明 |
|------|------|
| Linux **尽力** 在期限内调度 RT | 适合「大部分时候够快」 |
| **无硬实时绝对保证** | 中断关太久、内核 bug、硬件 SMI 仍可破期限 |
| 硬实时感更强 | `SCHED_DEADLINE` 或 `PREEMPT_RT` + 严格系统工程（见 [§4.2 六策略](./section-4.2-调度策略.md)） |

#### 用户态怎么设（概念）

| API / 工具 | 作用 |
|------------|------|
| **`sched_setscheduler` / `pthread_setschedparam`** | 设策略 + RT 优先级 |
| **`chrt -f/-r`** | 命令行改策略 |
| **`sched_setaffinity` / `taskset`** | 绑核（常与 RT 联用） |

需要 **`CAP_SYS_NICE`** 等能力；容器里还要看 cgroup 是否允许 RT。

#### RT 带宽 / 节流（现代内核，书外必知）

| 问题 | 现象 |
|------|------|
| 一个 FIFO 死循环占满核 | 同核 CFS（含 kthreads、部分管理路径）饿死 |
| 内核对策 | **RT 带宽限制**（如 `sched_rt_runtime_us`）— RT 不能 100% 吃光 |

**HFT 实盘清单：**

1. 热线程：`SCHED_FIFO` + 高但合理的 RT prio（不必人人 99）
2. **`sched_setaffinity` 绑隔离核**；网卡 IRQ / softirq 迁走或同策略规划
3. 同核不要再跑日志压缩、Java GC、无关容器
4. 留出管理核跑 CFS（ssh、监控、控制面）
5. 用 `cyclictest` / 业务直方图看 **尾延迟**，不要只看平均

**慎用：** 滥用 FIFO = 整机「假死」、RCU 回调饿、磁盘/网络慢路径崩。

→ [07 TLPI Ch 34–37](../../../03-linux-userspace-api/) · [4.7 syscall](./section-4.7-与调度相关的系统调用.md) · [17 HFT Practice](../../../16-hft-engineering/)

### 常见陷阱

1. 混淆 SCHED_FIFO 和 SCHED_RR——FIFO 无时间片（跑到阻塞），RR 有时间片轮转
2. 以为 RT 优先级 99 就是最高——SCHED_DEADLINE 优先级更高（EDF 算法）
3. 忽略 RT throttling——RT 线程默认被限制在 95% CPU 时间，HFT 需要禁用

---

<details>
<summary>自测题（点击展开）</summary>

**Q1.** SCHED_FIFO 和 SCHED_RR 的核心区别？

<details><summary>答案</summary>

FIFO：同优先级内先到先服务，无时间片，跑到阻塞/被更高优先级抢占/yield。RR：同优先级内轮转，每个任务有时间片（默认 100ms），片耗尽后轮到同优先级的下一个。HFT 通常用 FIFO——只有一个 RT 线程独占核，不需要轮转。

</details>

**Q2.** RT throttling 机制是什么？HFT 怎么处理？

<details><summary>答案</summary>

`/proc/sys/kernel/sched_rt_period_us`（默认 1s）和 `sched_rt_runtime_us`（默认 0.95s）限制 RT 线程在每 period 内最多跑 runtime 时间。超出后 RT 被节流，CFS 接管。HFT 设 `sched_rt_runtime_us=-1` 禁用节流。注意：禁用后如果 RT 线程死循环会锁死 CPU，设 `RLIMIT_RTTIME` 做安全网。

</details>

**Q3.** HFT 使用 SCHED_FIFO 的完整配置步骤？

<details><summary>答案</summary>

```c
// 1. 绑核
cpu_set_t cs; CPU_ZERO(&cs); CPU_SET(2, &cs);
sched_setaffinity(0, sizeof(cs), &cs);
// 2. RT 优先级 99
struct sched_param sp = { .sched_priority = 99 };
sched_setscheduler(0, SCHED_FIFO, &sp);
// 3. 锁内存
mlockall(MCL_CURRENT | MCL_FUTURE);
// 4. 内核启动参数: isolcpus=2 nohz_full=2 rcu_nocbs=2
```

</details>

</details>


> ↔ [ULK Ch7 §2 调度策略与抢占](../../../18-linux-kernel-deep/chapter-07-process-scheduling/notes/section-2-调度策略与抢占.md)
---
