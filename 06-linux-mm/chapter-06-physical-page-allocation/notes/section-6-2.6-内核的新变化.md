# Ch 6 §6 2.6 内核的新变化 → v6.6（pageset 的完整形态）

> **Understanding the Linux Virtual Memory Manager** · Mel Gorman · **选读 🟡**
> 源码核验：Linux **v6.6**（`include/linux/mmzone.h` per_cpu_pages、`mm/page_alloc.c` :1183/:2169/:2323）

---

## 本节讲什么

pageset（per-CPU page list）是 2.6 引入、至今仍是 order-0（及 v6.6 的 order≤3）分配的主干。本节给 v6.6 的 **完整结构**（含原书时代不存在的字段）、批量算法、和它与 SLUB cpu_slab 的体系对照。

---

## 1. v6.6 的 `per_cpu_pages`（mmzone.h 实锚）

```c
struct per_cpu_pages {
    spinlock_t lock;
    int count;              /* 链上页数 */
    int high;               /* 回吐水位：超过则批量还 buddy */
    int batch;              /* 每次与 buddy 交换的块大小 */
    short free_factor;      /* v6.6：free 压力下自动放大 batch */
#ifdef CONFIG_NUMA
    short expire;           /* v6.6：远端 pageset 的过期计数 */
#endif
    struct list_head lists[NR_PCP_LISTS];  /* 按 order×migratetype 分列！ */
};
```

| 与原书差异 | 说明 |
|------------|------|
| **hot/cold 双链已删除** | LIFO 已表达热度；分链是实测无效的过度设计（5.x 清理） |
| **order 0–3 都进 pcp** | `NR_LOWORDER_PCP_LISTS = MIGRATE_PCPTYPES × (PAGE_ALLOC_COSTLY_ORDER+1)`（mmzone.h:671）——不只是 order-0 |
| `free_factor` | free 侧 batch 自适应（内存压力下放大批、加快回吐） |
| `expire`（NUMA） | 本 CPU 上"远端 node"的 pageset 会过期清空，防远端页滞留 |

## 2. 批量算法（v6.6 实锚）

```
分配：rmqueue_pcplist()（:2169 附近）
    pcp 空 → 从 buddy 一次拿 batch 页（zone 锁一次）
释放：free_unref_page() → 本 CPU pcp（无锁）
    pcp->count > high → free_pcppages_bulk(zone, to_drain, pcp)（:1183）
                       按 migratetype 轮转挑链，批量 __free_one_page
```

| 参数 | 语义 | 调整 |
|------|------|------|
| `high` | 回吐阈值 | `percpu_pagelist_high_fraction` sysctl（0=默认按 zone 大小算） |
| `batch` | 交换批量 | 派生自 high（约 high/4，:2364 `min(batch<<2, high)` 关系可见） |

**设计权衡：** high 大 → zone 锁频率低，但页"藏"在 pcp 里对别的 CPU 不可见（NUMA 远端尤甚）；high 小 → 锁频繁。**per-CPU 缓存的通用两难**，SLUB 的 cpu_partial 同题同解。

## 3. 原书伏笔的正确收法：两级 per-CPU 串联

```
对象分配（kmalloc/kmem_cache_alloc）
  └─ SLUB cpu_slab（active slab 冻结）        ← 对象级，Ch 8
        └─ pcp pageset（order 0–3）            ← 页级，本节
              └─ zone free_area（buddy）        ← 全局
                    └─ node fallback → kswapd/compaction/OOM
```

| 层 | 交换单位 | 全局锁频率 |
|----|----------|------------|
| SLUB | 整 slab（64+ 对象） | 极低 |
| pcp | batch 页 | 低 |
| zone | 单块 | 每次慢路径 |

**每层把锁频率再除以一个量级**——这是 Linux 多核扩展的"套娃"答案，也是用户态多级池（每核热池→NUMA 冷池→全局）的蓝本。

## 4. 统一 NUMA API（原书 "What's New" 的另一条）

| 2.4 | 2.6+ / v6.6 |
|-----|-------------|
| UMA/NUMA 底层函数分叉 | `numa_node_id()` 隐式选 node；`alloc_pages_node()` 显式 |
| 应用无从干预 | `set_mempolicy()`/`mbind()`/`numactl` 覆盖（mpol 映射进 zonelist 过滤） |

HFT 的 NUMA 纪律照旧：**绑核+绑内存同 node**（线程绑 node0 核、内存 `mbind` node0），防 pageset 的 expire 机制把你推去远端。

## 5. 观测与调优

```bash
grep -w 'pgalloc\|pgfree\|allocstall' /proc/vmstat
# pcp 效率：zone 锁争用（perf）：
perf stat -e 'lock:*zone*' -a sleep 5 2>/dev/null || perf record -g -p <pid>
# 调整回吐阈值（谨慎）：
cat /proc/sys/vm/percpu_pagelist_high_fraction   # 0 = 自动
```

## 6. HFT 精读 checklist（章级收束）

| 现象 | 查什么 |
|------|--------|
| 远程 NUMA 延迟 | node-local 分配；`numastat`；mbind 策略 |
| latency 尖刺 | direct reclaim / compaction / GFP 上下文（allocstall/compact_stall） |
| 大页分配失败 | 外部碎片（pagetypeinfo）；pin 页阻碍 compaction |
| 与 slab/mempool | Buddy 管页；热路径对象必须池化（Ch 8 / DPDK） |
| pcp 参数 | percpu_pagelist_high_fraction（一般不动，知道有此旋钮即可） |

## 7. 衔接

- 全章：[§1](./section-1-空闲块的管理.md) [§2](./section-2-页面分配.md) [§3](./section-3-页面释放.md) [§4](./section-4-GFP-标志与进程标志.md) [§5](./section-5-避免碎片化.md)
- [Ch 2 §5 pageset 原书视角](../../chapter-02-describing-physical-memory/notes/section-5-2.6-内核的新变化.md)
- [Ch 8 §5 SLUB 的对象级 per-CPU](../../chapter-08-slab-allocator/notes/section-5-每-CPU-对象缓存.md)
- [06.5/ch01 memblock](../../../06.5-modern-mm/chapter-01-physical-memory-memblock/)：pcp/zone 的 boot 期初始化

---

<details>
<summary>代码自测（Q&A）</summary>

**Q1：v6.6 pcp 缓存 order 0–3，为什么 cutoff 恰在 3？**
A：`PAGE_ALLOC_COSTLY_ORDER=3`（mmzone.h:43）——order>3 的分配"贵"（低频+高失败率），缓存收益低；且高阶块应尽快回 buddy 触发合并（保连续性）。**"缓存高频小请求、直通低频大请求"**是分层缓存的通用划界法。

**Q2：pcp 的页对别的 CPU 可见吗？NUMA 远端 CPU 想要怎么办？**
A：不可见（本核私有）。远端 CPU 的分配会走自己 pcp→自己 node 的 buddy。唯一交互是 drain：CPU 热插拔/内存隔离/维护时 `drain_all_pages()` 强制清空。`expire` 字段管的是另一件事——**本 CPU 上属于远端 node 的 pageset** 定期过期，防页滞留错 node。

**Q3：为什么 free 也要批量回吐而不是立刻逐页还 buddy？**
A：逐页还 = 每页一次 zone 锁 + 一次合并尝试（伙伴可能不在，白拿锁）。攒到 high 一次性还：锁次数 ÷ batch，且批量还时伙伴互为邻居的几率高（刚分配的页空间局部性强），合并成功率也高。**攒批的收益=锁摊薄+合并概率提升，双份**。

**Q4：`free_factor` 什么时候会被推高？**
A：free 侧持续流入（如释放大内存）超过分配消耗时——pcp 快速逼近 high，此时放大 batch 让每次回吐带走更多页，减少"到 high-还一点-又到 high"的振荡。这是 v6.6 加的自适应阻尼，思想等同 TCP 拥塞窗口的加性增。

**Q5：用户态还有必要复刻 pcp 吗？是不是 hugepage 一次到位就够？**
A：分层仍有价值：hugepage 管大块（TLB/确定性），**页内切分还是池的事**——DPDK mempool 两层齐全（mempool 对象层 + memzone/大页层）正是 pcp+buddy 的用户态复刻。跳过对象层 = 每次分配浪费一整页或去抢全局锁。

</details>

---
