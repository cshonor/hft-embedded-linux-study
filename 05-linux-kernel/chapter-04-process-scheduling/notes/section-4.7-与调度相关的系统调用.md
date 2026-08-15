## ⑦ 与调度相关的系统调用 · Scheduling-Related System Calls

把 Ch 4 的策略落到用户态接口 — **调参入口**，不是另发明一套调度器。

#### 优先级与策略

| 接口（概念） | 作用 |
|--------------|------|
| **`nice` / `setpriority`** | 只调 **CFS** nice → weight / `vruntime`（对 RT **无效**） |
| **`sched_setscheduler` / `sched_getscheduler`** | 设/取策略（OTHER / **FIFO** / **RR**…）+ RT 时带 **`rt_priority`** |
| **`sched_setparam` / `sched_getparam`** | RT 优先级等参数 |
| **`sched_setattr`（现代）** | 统一设策略+参数（含 Deadline 等） |
| **`chrt -f/-r`** | 命令行设 FIFO/RR + 实时优先级 |

| 想改谁 | 用什么 |
|--------|--------|
| 普通进程份额 | `nice` / `renice` / `setpriority` |
| 变成 / 调整 RT | **`sched_setscheduler` 或 `chrt`**，设 `rt_priority`（越大越优先） |

#### CPU 亲和 · Affinity

| 接口 | 作用 |
|------|------|
| **`sched_setaffinity` / `sched_getaffinity`** | 限制任务可跑的 CPU 集合 |
| **`taskset`** | 命令行包装 |

```
未绑核：调度器可在多核间迁移 ──► 缓存冷、抖动
绑核：  任务只在 mask 内跑 ──► 可配合 isolcpus 做「专核」
```

| 注意 | 说明 |
|------|------|
| 子进程继承 | fork 后常继承 affinity；要确认线程池是否每人一核 |
| 与 IRQ 亲和 | `/proc/irq/*/smp_affinity` — 网卡中断也要规划，否则 softirq 仍砸热核 |

#### 主动让出与查询

| 接口 | 作用 |
|------|------|
| **`sched_yield`** | 主动让出；同优先级才有意义，**滥用伤性能** |
| **`sched_get_priority_max/min`** | 查策略允许的优先级范围 |
| **`sched_rr_get_interval`** | RR 时间片长度 |

#### `nanosleep` / 超时与调度

长时间睡眠走等待队列（4.4）；短延迟用户态常用忙等或 `clock_nanosleep` — 与 **Ch 11 定时器** 衔接。

#### HFT 常用组合（检查清单）

| 步骤 | 做什么 |
|------|--------|
| 1 | `sched_setaffinity` 绑隔离核 |
| 2 | `chrt -f` / `sched_setscheduler(SCHED_FIFO)` |
| 3 | 确认 IRQ/RPS/NAPI 不打到同核（Ch 7–8、Rosen） |
| 4 | `mlockall` 等减少缺页（Ch 15 / Gorman） |
| 5 | 测 P99/P999，不要只看均值 |

```c
/* 概念示意 — 生产用错误检查与权限处理 */
cpu_set_t set;
CPU_ZERO(&set);
CPU_SET(2, &set);
sched_setaffinity(0, sizeof(set), &set);

struct sched_param sp = { .sched_priority = 80 };
sched_setscheduler(0, SCHED_FIFO, &sp);
```

→ [07 TLPI](../../../03-linux-userspace-api/) · [4.6 RT](./section-4.6-实时调度策略.md) · [15 SysPerf](../../../14-systems-performance/)

### 常见陷阱

1. 混淆 nice() 和 setpriority()——nice() 是相对调整（+=inc），setpriority() 是绝对设置
2. 以为 sched_setaffinity() 需要 root——只需要 CAP_SYS_NICE 设置 RT 策略，affinity 任何进程都可以设
3. 在 SCHED_FIFO 下用 sched_yield()——FIFO 下 yield 移到同优先级队列末尾，可能不立即重新运行

---

<details>
<summary>自测题（点击展开）</summary>

**Q1.** nice(inc)、setpriority()、sched_setscheduler() 的区别？

<details><summary>答案</summary>

nice(inc)：当前进程 nice += inc，受 RLIMIT_NICE 限制。setpriority(PRIO_PROCESS, pid, prio)：设置指定进程的 nice 绝对值。sched_setscheduler(pid, policy, &param)：切换调度策略和 RT 优先级。HFT 用 sched_setscheduler(SCHED_FIFO, 99) 设最高 RT 优先级。

</details>

**Q2.** sched_setaffinity() 和 isolcpus 有什么区别？

<details><summary>答案</summary>

sched_setaffinity()：运行时设置进程可运行的 CPU 集合，其他进程仍可被调度到这些核。isolcpus=2：启动时从调度器可运行集合移除 2 号核，普通任务不会调度到 2，需要手动 taskset/sched_setaffinity 把 RT 线程放上去。isolcpus 更彻底（连 kworker/RCU 都不走），affinity 更灵活。HFT 两者都用。

</details>

**Q3.** 如何用 sched_getaffinity() 和 sched_setaffinity() 绑核？

<details><summary>答案</summary>

```c
#include <sched.h>
cpu_set_t mask;
CPU_ZERO(&mask);
CPU_SET(2, &mask);  // 绑到 2 号核
sched_setaffinity(0, sizeof(mask), &mask);  // 0=当前进程
// 验证
cpu_set_t get;
sched_getaffinity(0, sizeof(get), &get);
printf("CPU 2: %d\n", CPU_ISSET(2, &get));  // 1
```

</details>

</details>


> ↔ [ULK Ch7 §6 调度相关系统调用](../../../18-linux-kernel-deep/chapter-07-process-scheduling/notes/section-6-调度相关系统调用.md)
---
