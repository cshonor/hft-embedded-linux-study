# Ch 8 §5 每 CPU 对象缓存（SLUB 的 per-CPU 三件套）

> **Understanding the Linux Virtual Memory Manager** · Mel Gorman · **精读 🔴**
> 源码核验：Linux **v6.6**（`mm/slub.c` :54 lock 注释、:181 `this_cpu_ptr`、:340 `raw_cpu_inc`）

---

## 本节讲什么

多核扩展性的答案在原书里是 `array_cache`（每 CPU 批量借还）；SLUB 重构为 **active slab + per-CPU partial + tid 事务号** 三件套。本节讲清楚三者的容量语义和换入换出时机——它是"per-CPU 任何缓存"（pcp、mmap batch、io_uring provide batch）的通用范式。

---

## 1. 原书 array_cache → SLUB 三件套

| | 原书 array_cache（SLAB） | SLUB v6.6 |
|---|--------------------------|-----------|
| 热层形态 | 指针数组（batch 借还） | **整个 active slab**（一次冻结一整块） |
| 批量单位 | 对象 | slab（一个 slab 的全部对象） |
| 与全局交换 | array_cache.batch 个一批 | slab 用尽 → 换下一个 partial |
| 空闲管理 | 全局 partial 三链 | per-CPU partial 链 + node partial 两级 |
| 元数据 | array_cache 结构体 | `kmem_cache_cpu`（更小） |

**粒度变粗的智慧：** SLAB 按"对象"批交互（频繁少量进慢路径）；SLUB 按"slab"批交互（罕见大量）——**慢路径次数少一个数量级，均摊成本更低**。用户态池的"每次 refill 一页槽位而不是一个对象"同构。

## 2. active slab 生命周期

```
本 CPU 拿到新 active slab（freeze）
   │ freelist：obj₁→obj₂→…→obj_N（随机化后顺序）
   ▼ 分配：CAS 弹头，直到空
   空！→ deactivate 当前 slab（若全满直接丢；有余留挂 per-CPU partial）
   ▼
c->partial 链有备胎？→ 取一个当新 active（仍无全局锁）
没有？→ node->partial 搬（list_lock）
还没有？→ new_slab → buddy
```

| 阶段 | 锁 | 备注 |
|------|----|------|
| active 内分配/本 CPU free | **无锁 CAS** | :181 `this_cpu_ptr` 命中本核实例 |
| 换 active（本 CPU partial） | local lock（`cpu_slab->lock`，:54） | 关抢占即可 |
| 搬 node partial | `node->list_lock` | 唯一全局点 |
| 统计计数 | `raw_cpu_inc`（:340 注释：避免 this_cpu_add 的 irq-disable 开销） | 展示 per-CPU 计数器的开销意识 |

## 3. per-CPU partial 的容量控制

```c
kmem_cache->cpu_partial;   /* 每 CPU partial 链允许的对象总数（不是 slab 数） */
/sys/kernel/slab/<name>/cpu_partial
```

- free 端把"还有空位"的 slab 挂进 per-CPU partial（不用拿 list_lock）
- 超过 `cpu_partial` 上限 → **bulk 转移** 到 node partial（一次锁，搬一串）
- 默认值按对象大小反比（小对象多备、大对象少备，总内存近似守恒）

**设计要点：容量阈值统一以"对象数"计，让不同 size 的 cache 的 per-CPU 占用自动归一。** 用户态 per-core arena 的容量策略照抄即可。

## 4. 统计路径的开销洁癖（值得单独学）

```c
/* slub.c:338 注释原文翻译 */
/*
 * 追踪统计时用 this_cpu_inc() 会有 irq-disable 开销；
 * 统计允许丢少许精度 → raw_cpu_inc()：不禁中断，可被中断冲掉一次计数。
 */
```

**启示：** 热路径上的 per-CPU 计数器，问一句"这个数字需要精确吗"——不需要就 raw 版，省掉每次的中断关开（~20ns×每次分配）。HFT 指标系统同理：**遥测本身不该进延迟预算**。

## 5. 与 Ch 6 pageset 的并列（原书伏笔的正确收法）

| | Buddy pcp（Ch 6） | SLUB cpu_slab（本节） |
|---|-------------------|------------------------|
| 粒度 | 页 | 对象 |
| 换入单位 | batch 页 | 整个 slab |
| 上限 | `high` 水位（可调） | `cpu_partial` 对象数 |
| 越层去向 | zone free_area | node->partial |
| 锁 | zone->lock | node->list_lock |

**两级 per-CPU（页级+对象级）串联**，整条链路：对象分配 99% 停在 SLUB 快路径；它偶尔的 slab 补给大多停在 buddy pcp；再往下才是 zone。**每层 per-CPU 都把全局锁频率除以一个量级。**

## 6. 观测

```bash
grep -w 'cpu_partial' /sys/kernel/slab/*/ -r 2>/dev/null | head
cat /sys/kernel/slab/<name>/objects_partial
# 每 CPU 视角（需 slub_debug）：
cat /sys/kernel/slab/<name>/slabs_cpu_partial
```

## 7. HFT 用户态镜像（收束成一张实施表）

| SLUB 机制 | 用户态对应 |
|-----------|-----------|
| active slab 冻结 | 每核 bump 区 + freelist 备区 |
| per-CPU partial 上限 | 每核 arena 的 refill 阈值（如 512 槽） |
| node partial bulk 转移 | 全局池按批收割各核余量（一次锁搬一批） |
| tid 防 ABA | versioned CAS |
| raw_cpu_inc 计数 | 遥测放慢路径或用宽松原子 |

```
Core 0:  local Order[64]  ── CAS pop/push 无锁
         空了 → 从 global pool 一次拿 32 个（bulk）
Core 1:  同上
```

## 8. 衔接

- [§2 数据结构](./section-2-核心数据结构：Cache-与-Slab.md) / [§3 瀑布](./section-3-对象分配与释放.md)
- [Ch 6 §6 pageset](../../chapter-06-physical-page-allocation/)：页级 per-CPU
- [06.5/ch02](../../../06.5-modern-mm/chapter-02-slab-slub-allocator/)

---

<details>
<summary>代码自测（Q&A）</summary>

**Q1：为什么 SLUB 的 per-CPU 慢路径比 SLAB 少一个量级？**
A：粒度。SLAB array_cache 批量是"对象数"（如 32 个），用完就回全局；SLUB 一次冻结整 slab（64~256 个对象且连带 free 免回全局），慢路径频率 ÷ slab 大小。**per-CPU 层的容量决定它的"续航力"**——用户态池的 refill batch size 是同款参数。

**Q2：active slab 的对象被别的 CPU free 回来，per-CPU 计数还准吗？**
A：frozen 语义保证对象仍归这个 slab，跨核 free 只是 CAS 挂回 slab->freelist（不进任何 per-CPU 结构）。所以 `slab->inuse` 等计数是 slab 级原子——per-CPU 统计只管"本 CPU 快路径次数"，不试图精确追踪对象归属。**统计口径与结构边界对齐**是并发设计的纪律。

**Q3：`cpu_partial` 调大有什么代价？**
A：每 CPU 常驻内存 = Σ(cache) × cpu_partial × object_size × NR_CPUS。16KB 对象的 cache 配 30 备胎 × 96 核 ≈ 45MB 只在 partial 上睡觉。HFT 内核态组件（自研驱动）调优时按"每秒换主次数 × refill 成本"倒推，别拍脑袋加倍。

**Q4：中断上下文里能走 SLUB 快路径吗？**
A：能——CAS 快路径不禁中断不睡眠，中断里 `kmalloc(GFP_ATOMIC)` 大概率命中本 CPU freelist 直接返回。只有掉到 ③ 层（buddy）才受 GFP_ATOMIC 约束。这也是"快路径必须可重入/无睡眠"的活例子。

**Q5：用户态 per-core 池的"归还错核"问题，SLUB 怎么绕开的？**
A：SLUB 不搬对象，只搬 slab：对象 free 回原 slab（跨核也便宜），slab 整块在层间转移。用户态照抄：**不要迁移对象，迁移整块内存的所有权**（arena 手递手 transfer，对象永远呆在出生地）——这是 NUMA 友好且免 128B 对象跨核搬运的做法。

</details>

---
