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

→ [07 TLPI Ch 34–37](../../../../04-linux-userspace-api/) · [4.7 syscall](./section-4.7-与调度相关的系统调用.md) · [17 HFT Practice](../../../../21-hft-engineering/)

---
