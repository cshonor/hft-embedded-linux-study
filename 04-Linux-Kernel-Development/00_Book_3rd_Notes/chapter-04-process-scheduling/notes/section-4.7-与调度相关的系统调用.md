## ⑦ 与调度相关的系统调用 · Scheduling-Related System Calls

把 Ch 4 的策略落到用户态接口 — **调参入口**，不是另发明一套调度器。

#### 优先级与策略

| 接口（概念） | 作用 |
|--------------|------|
| **`nice` / `setpriority`** | 调 CFS **nice** |
| **`sched_setscheduler` / `sched_getscheduler`** | 设/取策略（OTHER/FIFO/RR…） |
| **`sched_setparam` / `sched_getparam`** | RT 优先级等参数 |
| **`sched_setattr`（现代）** | 统一设策略+参数（含 Deadline 等） |

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

→ [07 TLPI](../../../../07-The-Linux-Programming-Interface/) · [4.6 RT](./section-4.6-实时调度策略.md) · [15 SysPerf](../../../../15-Systems-Performance-2nd/)

---
