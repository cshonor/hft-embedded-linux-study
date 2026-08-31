# Ch 6 §2 页面分配（rmqueue 与慢路径瀑布）

> **Understanding the Linux Virtual Memory Manager** · Mel Gorman · **选读 🟡**
> 源码核验：Linux **v6.6**（`mm/page_alloc.c` :1565 `__rmqueue_smallest`、:1597 `fallbacks[]`、:2106 steal 路径）

---

## 本节讲什么

"从 free_area 取一块 order-k"的完整决策链：**本类型直取 → 高阶拆分 → 迁移类型 fallback（偷页）→ zone/node fallback → 慢路径回收**。每往下一层，代价上一个台阶——这张"代价阶梯"就是 HFT 分配延迟分析的地图。

---

## 1. 快路径：`__rmqueue_smallest`（page_alloc.c:1565）

```
请求 (zone, order, migratetype)
for (o = order; o <= MAX_ORDER-1; o++) {
    块 = free_list[o][mt] 取头;
    if (块) {
        expand(zone, 块, o → order);   /* 拆分：一半交付，其余逐级挂回 free_list */
        return 块;
    }
}
return NULL;  → 进 fallback
```

**expand 拆分**：从 order-9 拆到 order-0 时，产生 9 个伙伴块挂回各级链——全部挂 **同 migratetype** 链上。

**注意**：order ≤ 3 的用户态路径实际先走 pcp（§6），`__rmqueue_smallest` 是 pcp miss 后的补给层。

## 2. 迁移类型 fallback：偷页（steal）

本类型无空闲时查 **fallbacks 表**（page_alloc.c:1597 实锚）：

```c
static int fallbacks[MIGRATE_TYPES][MIGRATE_PCPTYPES - 1] = {
    [MIGRATE_UNMOVABLE]   = { MIGRATE_RECLAIMABLE, MIGRATE_MOVABLE   },
    [MIGRATE_MOVABLE]     = { MIGRATE_RECLAIMABLE, MIGRATE_UNMOVABLE },
    [MIGRATE_RECLAIMABLE] = { MIGRATE_UNMOVABLE,   MIGRATE_MOVABLE   },
};
```

偷页规则（`rmqueue_fallback`，:2106 附近）：

| 请求 order | 偷法 | 后果 |
|-----------|------|------|
| < pageblock_order | 只偷够用的页（`can_steal_fallback` 判定） | pageblock 保持原类型标签 → **混居** |
| ≥ pageblock_order（如 THP） | **整块偷**：pageblock 换标签 | 整块变 UNMOVABLE——永久性碎片化 |

**HFT 记住：UNMOVABLE 高阶请求是碎片化的头号制造者。** 长跑机器上 THP/大页失败率上升的元凶常常是早期某驱动反复高阶 kmalloc。

## 3. Zone / Node fallback：zonelist

| 层级 | 行为 |
|------|------|
| 首选 zone（`GFP` 的 zone 掩码 + `highest_zoneidx`） | NORMAL → DMA32 → DMA 降序尝试（`GFP_HIGHUSER` 为例） |
| 本 node 首选 zone 水位不足 | zonelist 上下一个 node 的 zone（**remote 分配**，延迟 +~40-100ns 访存差） |
| `cpuset`/`mempolicy` 生效时 | zonelist 被策略过滤——`MPOL_BIND` 把 node 锁死 |

水位判定：`zone_watermark_fast()`（:2920）快速判断"该 zone 还够不够 + 这次 order 要预留多少"——高 order 判定更苛刻（要满足 order 阶的 fragmentation reserve）。

## 4. 慢路径瀑布（`__alloc_pages_slowpath`）

快路径全线失败（`__alloc_pages_noprof` 概念路径）：

```
① 降低标准重试（ALLOC_WMARK_MIN、放开 ALLOC_CPUSET） 
② 唤醒 kswapd（异步回收，自己先跑）
③ direct reclaim：同步释放页（调用者陷入回收，µs~ms！）
④ direct compaction：同步搬迁页面凑连续块（THP 大头，最高毫秒级！）
⑤ __GFP_NORETRY？认输降级 / OOM（Ch 13）
```

| 步 | 用户态可感症状 |
|----|----------------|
| ③ direct reclaim | `allocstall`/`allocstall_*` vmstat 计数 +1；进程停在 `__alloc_pages` 的栈 |
| ④ compaction | `compact_stall` +1；THP 场景 `thp_deferred` 相关；尾延迟尖刺 |
| ⑤ OOM | 除非 `__GFP_NORETRY`/`GFP_ATOMIC` |

**HFT 铁律映射：** 用户态 mlock+prefault+大页预留在做的事 = **让引擎运行期永远停在快路径**，③④⑤ 全部转移到启动期。

## 5. GFP → alloc_flags 的翻译

`GFP_KERNEL` 之类是"对外语言"，进入分配器先翻译成对内 flags：

| alloc_flags | 意义 |
|-------------|------|
| `ALLOC_WMARK_MIN/LOW/HIGH` | 用哪条水位线放行 |
| `ALLOC_HARDER` | 高阶请求的额外余地（配合 MIGRATE_HIGHATOMIC） |
| `ALLOC_CPUSET` | 尊重 cpuset 限制（低水位后放开换生存） |
| `ALLOC_OOM` | OOM 允许的最后一搏 |

同一个 GFP_KERNEL 在水位健康/紧张时行为不同——**分配器的"策略梯度"是运行时状态决定的**。

## 6. HFT / 嵌入式关联

| 现象 | 本节机制 |
|------|----------|
| `numastat` 出现远程分配 | zonelist 走到 remote node（bind 策略没锁死或本 node 水位不足） |
| 尾延迟尖刺 + `compact_stall` | direct compaction——检查 THP defrag 策略与内存余量 |
| 高阶分配长期失败 | §2 偷页留下的 UNMOVABLE 混居，pagetypeinfo 验证 |
| 引擎启动慢但运行稳 | prefault/预留把 ③④ 转移到启动期的正确姿势 |

## 7. 衔接

- [§1 空闲块的管理](./section-1-空闲块的管理.md)：结构基础
- [§3 页面释放](./section-3-页面释放.md)：对称的一半
- [§4 GFP](./section-4-GFP-标志与进程标志.md)：慢路径开关
- [Ch 10 回收](../../chapter-10-page-frame-reclamation/)：③ 的内部
- [06.5/ch07 MGLRU](../../../06.5-modern-mm/chapter-07-page-reclaim-mglru/)：回收的现代形态

---

<details>
<summary>代码自测（Q&A）</summary>

**Q1：为什么 MOVABLE 偷页比 UNMOVABLE 偷页危害小？**
A：偷来的 MOVABLE 页将来可以被 compaction 迁走复原；UNMOVABLE 页（内核直映/slab）钉死在偷来的位置上，pageblock 的原类型布局永久破坏。fallbacks 表里三个类型互为后备，但生态代价不对称。

**Q2：请求 order-9（THP）失败但 free 内存充足，为什么？**
A："free 充足"是 order-0 视角。order-9 需要 **连续 512 页且同 migratetype 的 free_list 有货**。碎片化后 MOVABLE 的 order-9 链空——只能靠 compaction 现场凑。`pagetypeinfo` 的 order-9 行比 `/proc/meminfo` 的 MemFree 更接近真相。

**Q3：`GFP_ATOMIC` 失败返回 NULL 前做了什么挣扎？**
A：走快路径 + 水位降到 MIN + 偷 HIGHATOMIC 储备（alloc_flags 含 ALLOC_HARDER 语义）——但 **不做** reclaim/compaction（不可睡眠）。所以 GFP_ATOMIC 失败 ≠ 内存耗尽，只等于"无锁可拿的连续块没了"。中断里的分配者必须处理 NULL。

**Q4：zonelist 的顺序为什么 NORMAL 优先于 DMA？**
A：ZONE_NORMAL 又大又快（无 DMA 限制）；ZONE_DMA 稀缺（16MiB 以下的 ISA 时代遗产），留给真需要低地址的设备。倒过来会瞬间耗尽 DMA 区。**"越专的池越往后放"是通用资源排程原则。**

**Q5：怎么用 perf/bpftrace 抓"谁触发了 direct compaction"？**
A：`perf record -e mm_compaction_try_to_compact_pages`；或 bpftrace 挂 kprobe:`try_to_compact_pages` 打印 comm+order。跑压测时定位到 THP 缺页（khugepaged 关掉却还有 compaction → 检查 gfp 里谁带了 `__GFP_DIRECT_RECLAIM` 高阶请求，常是某驱动）。

</details>

---
