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

---
