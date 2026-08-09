## ⑩ 每个 CPU 的分配 · Per-CPU

SMP 下 **全局变量 + 锁** 保护计数器 → **缓存行 bouncing** — **per-CPU 数据** 给 **每个 CPU 一份副本**，本核 **通常无锁写**。

#### 动机

| 问题 | per-CPU 解法 |
|------|--------------|
| **`atomic_t` 热点** | 每核 **私有计数** — 周期性汇总 |
| **锁竞争** | 本 CPU **只写 `__get_cpu_var(x)`** |
| **false sharing** | 每副本 **独立 cache line**（`____cacheline_aligned_in_smp`） |

```
传统:
  CPU0 ──┐
  CPU1 ──┼──► [ global counter ]  ← 同一 cache line 乒乓
  CPU2 ──┘

per-CPU:
  CPU0 ──► counter[0]
  CPU1 ──► counter[1]
  CPU2 ──► counter[2]
```

#### 接口（2.6+ 概念）

| 宏 / API | 作用 |
|----------|------|
| **`DEFINE_PER_CPU(type, name)`** | **静态** per-CPU 变量 |
| **`DECLARE_PER_CPU` / `per_cpu(var, cpu)`** | 声明 / 指定 CPU 访问 |
| **`__get_cpu_var(name)`** | **当前 CPU** 的实例 |
| **`get_cpu_var` / `put_cpu_var`** | **关抢占** 期间安全访问 |
| **`alloc_percpu(type)`** | **动态** 分配 per-CPU 对象 |
| **`free_percpu`** | 释放 |

```c
DEFINE_PER_CPU(unsigned long, irq_count);

void inc_irq_count(void)
{
    __get_cpu_var(irq_count)++;
    /* 读其他 CPU: per_cpu(irq_count, cpu) — 需 RCU/preempt 规则 */
}
```

#### 使用规则

| 规则 | 原因 |
|------|------|
| **访问本 CPU 数据时禁止抢占迁移**（或用 `get_cpu_var`） | 否则 **写到别核副本** |
| **汇总全系统值** | 遍历 **`for_each_possible_cpu`** — **非热路径** |
| **与 `preempt_disable`** | Ch 10 — per-CPU + **关抢占** 常结对 |

#### 典型内核用户

| 子系统 | per-CPU 内容 |
|--------|--------------|
| **softirq / NAPI** | **`softnet_data`** — 每核网络输入队列 |
| **Slab** | **cpu partial slab list** |
| **RCU** | **grace period 状态** |
| **统计** | **`vm_event_states`** 等 |

**HFT：** 用户态 **每核一条 SPSC ring**、**thread-local 订单簿缓存** = **per-CPU 同构**。避免 **`std::atomic` 全局 hot counter** — 用 **`cpu_local`** 聚合。与内核一样：**读总和慢路径做**，**写路径本核独占**。

→ [Ch 8 softirq per-CPU](../../chapter-08-bottom-halves/) · [Ch 10 preempt_disable](../../chapter-10-kernel-synchronization/) · [06 Gorman Slab per-CPU cache](../../../../06-linux-mm/chapter-08-slab-allocator/notes/section-5-每-CPU-对象缓存.md)



<details>
<summary>自测题（点击展开）</summary>

**Q1.** per-CPU 变量如何避免锁？有什么局限？

<details><summary>答案</summary>

per-CPU 变量给每个 CPU 一份独立副本，本核读写自己的副本无需锁。统计时累加所有 CPU 副本。局限：1) 抢占关闭期间才能安全访问本核副本（否则被迁移到其他核）；2) 累加需要遍历所有 CPU；3) 不能用于需要全局一致性的场景。网络收包计数器用 per-CPU，完美匹配单核收包模型。

</details>

**Q2.** 缓存行 bouncing 是什么？per-CPU 如何解决？

<details><summary>答案</summary>

多核频繁写同一全局变量（如计数器）→ 每个 CPU 的 L1 cache line 都要 invalidate → L2/L3 来回传递 cache line（bouncing）。per-CPU 给每个 CPU 独立计数器，本核写自己的 cache line 不影响其他核。只有读取总数时才汇总。这就是 `/proc/stat` 的实现原理。

</details>

</details>
---
