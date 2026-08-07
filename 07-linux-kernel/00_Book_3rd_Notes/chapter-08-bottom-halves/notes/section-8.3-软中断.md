## ③ 软中断 · Softirqs

**softirq（软中断）** 是 Linux **性能最关键** 的下半部机制 — **编译期静态定义**、**可多 CPU 同类型并行**，网络栈与块层大量依赖它。

| 属性 | 说明 |
|------|------|
| **分配** | **编译期静态** — `open_softirq()` 注册 handler |
| **数量** | 固定几种类型（`enum`）— 如 `NET_RX`、`NET_TX`、`BLOCK`、`TIMER` |
| **触发** | `raise_softirq()` / `__raise_softirq()` — 常由 **hardirq** 调用 |
| **执行时机** | **中断返回路径** `irq_exit()` → `do_softirq()`；过载时 **`ksoftirqd`** |
| **上下文** | **软中断上下文** — **仍不能睡眠** |

#### 常见 softirq 类型（书中 + 实践）

| 类型 | 典型用途 |
|------|----------|
| **`HI_SOFTIRQ`** | **高优先级** tasklet 所在 |
| **`TIMER_SOFTIRQ`** | 定时器软中断处理 |
| **`NET_TX_SOFTIRQ`** | 网络发送 |
| **`NET_RX_SOFTIRQ`** | **网络接收** — HFT 热点 |
| **`BLOCK_SOFTIRQ`** | 块层完成 |
| **`TASKLET_SOFTIRQ`** | 普通 tasklet |

#### 执行流程（概念）

```
ISR: raise_softirq(NET_RX_SOFTIRQ)
        │
        ▼
irq_exit() 发现 pending
        │
        ▼
__do_softirq()  ──► net_rx_action() 等
        │                │
        │                └── NAPI poll · 协议栈
        ▼
若仍 pending 且超过 budget ──► ksoftirqd/n 继续
```

#### 并发模型 — 与 tasklet 的核心区别

| 对比 | softirq | tasklet |
|------|---------|---------|
| **同类型多 CPU 并行** | **可以** — 每 CPU 各跑 NET_RX | **不可以** — 同一 tasklet 全局串行 |
| **锁要求** | **高** — 共享队列必须严密同步 | **较低** — 同类 tasklet 不会并发 |
| **适用** | **核心子系统**、已充分多线程化 | **普通驱动** |

```
CPU0: NET_RX softirq ──┐
CPU1: NET_RX softirq ──┼──► 可能同时处理不同包
CPU2: NET_RX softirq ──┘
         │
         └── 共享 input_pkt_queue 等 → spinlock / per-CPU 队列
```

#### 开发者义务

| 手段 | 原因 |
|------|------|
| **`spinlock_t`** | 保护跨 CPU 共享结构 |
| **per-CPU 变量** | 统计、临时缓冲 — 无锁或减争用 |
| **NAPI budget** | 限制一次 softirq **处理包数** — 防饿死用户态 |
| **禁止睡眠 API** | softirq 不是进程上下文 |

```c
/* 内核子系统注册（驱动一般不直接 open_softirq） */
open_softirq(NET_RX_SOFTIRQ, net_rx_action);

/* hardirq 或协议栈内触发 */
raise_softirq(NET_RX_SOFTIRQ);
```

#### softirq 风暴

| 症状 | 原因 |
|------|------|
| **`%soft` 接近 100%** | 网络 flood、每包 raise NET_RX |
| **用户态饿死** | softirq 在 interrupt return 里跑太多 |
| **`ksoftirqd` RUNNING** | 溢出到辅助线程（Ch 8.6） |

**HFT：** 行情 ** multicast / 逐笔 flood** 时，`perf top` 常见 **`net_rx_action`**。调优：**中断合并**、**NAPI weight**、**RPS Spread**、**XPS**、或 **内核旁路（DPDK/AFP）**。`mpstat` 的 **`%soft`** 与 **`%irq`** 分开看。

→ [Ch 8.6](section-8.6-ksoftirqd-辅助线程.md) ksoftirqd · [Ch 8.4](section-8.4-tasklet.md) tasklet 对比 · [12 Rosen Ch14 NAPI/softirq](../../17-kernel-networking/chapter-14-advanced-topics/)

### 常见陷阱

1. 在 softirq 中调用睡眠函数——softirq 不能 schedule()，只能用 spinlock
2. 以为 softirq 可以动态注册——softirq 是编译时静态注册的（10 种类型），不能运行时添加
3. 混淆 softirq 的执行时机——在 hard IRQ 返回时 + ksoftirqd 内核线程中执行

---

<details>
<summary>自测题（点击展开）</summary>

**Q1.** softirq 的 10 种类型中哪些和 HFT 相关？

<details><summary>答案</summary>

HI_SOFTIRQ：高优先级 softirq（tasklet_hi 用）。TIMER_SOFTIRQ：定时器到期。NET_TX_SOFTIRQ / NET_RX_SOFTIRQ：网络收发（HFT 最关注）。BLOCK_SOFTIRQ：块 I/O 完成。TASKLET_SOFTIRQ：tasklet。SCHED_SOFTIRQ：负载均衡。HRTIMER_SOFTIRQ：高精度定时器。RCU_SOFTIRQ：RCU 回收。HFT 主要受 NET_RX 和 TIMER 影响。

</details>

**Q2.** softirq 的执行时机和机制？

<details><summary>答案</summary>

① hard IRQ 返回时：`__do_softirq()` 检查 pending softirq，执行最多 10 个 + 2ms 时间限制。② ksoftirqd：如果有持续 pending 的 softirq，唤醒 ksoftirqd 内核线程处理。③ 显式触发：`raise_softirq(NR_SOFTIRQ)`。softirq 在 per-CPU 上执行，同类型不会在多个 CPU 上并发。

</details>

**Q3.** HFT 如何减少 softirq 对交易线程的影响？

<details><summary>答案</summary>

① RPS（Receive Packet Steering）：`/sys/class/net/eth0/queues/rx-0/rps_cpus` 把收包 softirq 迁移到非交易核。② NAPI：网卡在中断后切换到轮询模式，减少中断+softirq 频率。③ `nohz_full`：隔离交易核的 softirq。④ DPDK：完全绕过内核网络栈，用户态轮询。⑤ `cat /proc/softirqs` 监控 softirq 频率。

</details>

</details>


> ↔ [ULK Ch4 §7 可延迟函数与工作队列](../../../../08-linux-kernel-deep/chapter-04-interrupts-and-exceptions/notes/section-7-可延迟函数与工作队列.md)
---
