# Ch 6 §1 空闲块的管理（Buddy 与迁移类型）

> **Understanding the Linux Virtual Memory Manager** · Mel Gorman · **选读 🟡**
> 源码核验：Linux **v6.6**（`mm/page_alloc.c` :1565/:1597、`include/linux/mmzone.h` :29/:226）

---

## 本节讲什么

Buddy 分配器的地基：**按 2 的幂组织空闲块**。原书讲 free_area + 伙伴位图；v6.6 在同一骨架上加了 **迁移类型（migratetype）维度**——这是抗碎片化战争的支柱，也是 HFT 大页稳定性的前提。

---

## 1. 阶（Order）与 free_area

物理空闲页按 **连续 2^k 页** 组织成块，`k` = order：

| Order | 块大小 | 用途 |
|-------|--------|------|
| 0 | 1 页 | 万物起点（pcp、slab） |
| 3 | 8 页 = 32KiB | `PAGE_ALLOC_COSTLY_ORDER`（mmzone.h:43）——超过此阶的失败视为"贵" |
| 9 | 512 页 = 2MiB | THP / hugetlb 的目标阶 |
| MAX_ORDER-1 | — | `MAX_ORDER=10` 默认（mmzone.h:29，ARM64 可到 10/13 因页大小不同） |

每个 zone 维护 **每 order 一组 free_list**：

```
zone->free_area[order].free_list[MIGRATE_UNMOVABLE]  ← v6.6：按迁移类型再分列！
                       .free_list[MIGRATE_MOVABLE]
                       .free_list[MIGRATE_RECLAIMABLE]
                       .free_list[MIGRATE_CMA / HIGHATOMIC ...]
                       .nr_free
```

**原书的"每 order 一条链"在 v6.6 是"每 order × 每 migratetype 一条链"。** 为什么要分列——见 §5，先记住结构。

## 2. 伙伴（Buddy）的数学

伙伴定义纯位运算：

```
buddy_pfn = pfn ^ (1 << order)      /* 翻转 order 位 = 另一半 */
合并结果 pfn = pfn & ~(1 << order)  /* 清掉 order 位 = 父块 */
```

| 原书：伙伴位图 | v6.6 |
|----------------|------|
| 每 **对** 伙伴 1 bit（0=同态，1=半空半满） | `__free_one_page`（page_alloc.c:763）**现算 XOR** 判伙伴 + 查其 `PageBuddy()` 标志确认空闲 |

位图省的是内存，现算省的是维护——内存不再稀缺后内核选了代码更简单的路。`struct page` 里 `page_type` 复用存 buddy 标志和 order（`PageBuddy`/`buddy_order`）。

## 3. 迁移类型（migratetype）——v6.6 的第二个维度

物理内存切成 **pageblock**（`pageblock_order`，page_alloc.c:226——x86_64 默认 order-9 即 2MiB），每块打一个迁移类型标签：

| 类型 | 含义 | 谁的页 |
|------|------|--------|
| `MIGRATE_UNMOVABLE` | 移动 = 改 PTE 都不行（直接映射、内核数据） | slab、kmalloc |
| `MIGRATE_MOVABLE` | 可迁移（改 PTE 搬家） | 用户匿名页、page cache |
| `MIGRATE_RECLAIMABLE` | 可回收（释放即可） | inode/dentry 缓存 |
| `MIGRATE_CMA` | CMA 保留区（设备 DMA 专用，空闲时可借给 MOVABLE） | 相机/媒体预留 |
| `MIGRATE_HIGHATOMIC` | 高阶请求预留 | 抗高阶 alloc 抖动 |
| `MIGRATE_ISOLATE` | 迁移中隔离 | compaction/内存热插拔 |

**一张图理解抗碎片逻辑：**

```
物理内存（pageblock 视角）：
┌──────┬──────┬──────┬──────┬──────┬──────┐
│ UNM  │ MOV  │ MOV  │ UNM  │ MOV  │ MOV  │
└──────┴──────┴──────┴──────┴──────┴──────┘
  ↑ MOVABLE 页集中放 → UNMOVABLE 不会打散它们
  → MOV 区内大块连续性容易保住 → 高阶分配(THP)有地可拿
```

不分类的后果：不可移动页像钉子一样随机钉进每个 2MiB 块，全局碎片化后 **2MiB 连续空闲永远凑不齐**——这正是 §5 主题。

## 4. pcp 视角的 free_list（v6.6 细节）

`__rmqueue_smallest()`（page_alloc.c:1565）从 zone 链表取块；order≤3 的分配 **不碰 zone 链**，走 per-CPU pageset（§6 详述）——pcp 的 list 也按 migratetype 分列（`NR_PCP_LISTS`）。

## 5. 观测

```bash
cat /proc/pagetypeinfo        # 每 zone 每 order 每 migratetype 的空闲块数 —— 碎片化体检表
grep -w 'pgfree\|pgalloc' /proc/vmstat
cat /sys/devices/system/node/node*/vmstat | grep -i pgalloc
```

`pagetypeinfo` 的 **order-9 行 MOVABLE 列归零** = 这台机器 THP/大页即将开始失败的先兆。

## 6. HFT / 嵌入式关联

| 机制 | 兑现 |
|------|------|
| 大页失败预警 | pagetypeinfo 高阶行监控进告警 |
| 引擎页的类型归属 | 用户态引擎几乎全 MIGRATE_MOVABLE（可被 compaction 搬走）——**mlock/pin 的页例外**：迁移被阻 → 引擎长期运行后自己制造 UNMOVABLE 孔洞 |
| pageblock_order = 2MiB | THP 的阶恰好钉在 pageblock 上——一个 pageblock 一致才能整块给大页 |
| 树莓派 | 小内存下 CMA 预留（视频编解码）挤占 MOVABLE——开机 `cat /proc/pagetypeinfo` 看 CMA 行 |

## 7. 衔接

- [§2 页面分配](./section-2-页面分配.md)：在这套结构上取块
- [Ch 2 物理内存](../../chapter-02-describing-physical-memory/)：zone/node 的上层组织
- [06.5/ch01](../../../06.5-modern-mm/chapter-01-physical-memory-memblock/)：boot 期这些结构怎么初始化

---

<details>
<summary>代码自测（Q&A）</summary>

**Q1：为什么伙伴用 XOR 翻转位而不是"地址+块大小"？**
A：块的起始地址不一定是"低半"（pfn 可能是父块的高半）。`pfn ^ (1<<order)` 对两种情况统一：低半翻成高半、高半翻成低半。用 + 会把高半算到父块外面去。

**Q2：一个 pageblock 里能混两种 migratetype 吗？**
A：能，且常见：fallback 借页（§2）会把 UNMOVABLE 请求放进 MOVABLE pageblock，产生"混居"。长期碎片化的机器大量混居。**混居块的大页梦想已碎**——恢复要靠 compaction 把 MOVABLE 搬走 + 释放借来的页。

**Q3：`MIGRATE_CMA` 空闲时借给谁？**
A：MOVABLE（`__rmqueue_cma_fallback`，page_alloc.c:1607 附近）——CMA 只敢借给可迁移页：设备要用时把借住者迁走即可腾出连续区。UNMOVABLE 借了就赖着不走，CMA 即失效。

**Q4：`PageBuddy()` 标志存哪？为什么够用？**
A：`struct page` 的 `page_type`/flags 位复用：空闲块的头页打 buddy 标志并记 order。够用的原因：**只有空闲块才需要在 free_list 上被识别**，分配出去的页该字段让给业务状态。元数据寄生思想的又一例（同 Ch 3 的 freelist 嵌对象）。

**Q5：`PAGE_ALLOC_COSTLY_ORDER=3` 的"贵"贵在哪？**
A：超过 order-3 的分配失败概率急剧上升（连续 32KiB+ 难找），重试与回收的代价不成比例。语义：≥此阶的 `__GFP_NORETRY` 分配失败要快速认输走降级路径（如 THP 退 4K、kvmalloc 退 vmalloc）。设计用户态 arena 时同款阈值思想：**"多大算大"决定降级策略**。

</details>

---
