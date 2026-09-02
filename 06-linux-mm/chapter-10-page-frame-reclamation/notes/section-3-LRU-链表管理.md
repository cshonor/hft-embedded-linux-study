# Ch 10 §3 LRU 链表管理

> **Understanding the Linux Virtual Memory Manager** · Mel Gorman · **选读 🟡**
> 源码核验：Linux **v6.6**（`mm/vmscan.c` 的 `shrink_inactive_list`、`mm/swap.c` 的 `lru_add_fn`）

---

## 本节讲什么

策略（§1）决定「扫哪条链」，本节讲「**具体怎么扫、怎么回收、怎么放回**」。核心是 v6.6 `shrink_inactive_list()`（`vmscan.c:2568`）的**三步走**。

原书函数 `refill_inactive()` / `shrink_cache()` 已不在 v6.6——现代实现是 `isolate_lru_folios()` → `shrink_folio_list()` → `move_folios_to_lru()`。

---

## 1. 三步走：isolate → shrink → move back

```c
static unsigned long shrink_inactive_list(unsigned long nr_to_scan,
        struct lruvec *lruvec, struct scan_control *sc, enum lru_list lru)
{
    LIST_HEAD(folio_list);
    /* ① 隔离：从 LRU 摘下一批 victim 到临时列表 */
    spin_lock_irq(&lruvec->lru_lock);
    nr_taken = isolate_lru_folios(nr_to_scan, lruvec, &folio_list,
                                  &nr_scanned, sc, lru);   /* vmscan.c:2304 */
    spin_unlock_irq(&lruvec->lru_lock);

    /* ② 收缩：逐个 folio 决策——丢 / 写回 / swap */
    nr_reclaimed = shrink_folio_list(&folio_list, pgdat, sc, &stat, false);

    /* ③ 放回：幸存者（没被回收的）放回 LRU，回收的释放 */
    spin_lock_irq(&lruvec->lru_lock);
    move_folios_to_lru(lruvec, &folio_list);
    spin_unlock_irq(&lruvec->lru_lock);
    free_unref_page_list(&folio_list);   /* 归还 Buddy */
    return nr_reclaimed;
}
```

**为什么先隔离再处理？** 隔离（isolate）阶段只持 `lru_lock` 极短时间，把 victim 摘到**进程私有临时列表**后立刻放锁；真正费时的「逐个 folio 决策」（可能要写盘、等 I/O）在**锁外**做。这避免了「一边扫描一边写盘还占着大锁」。

---

## 2. per-folio 决策：`shrink_folio_list()`

| folio 状态 | 动作 |
|-----------|------|
| 干净文件页、无映射 | 直接 free 回 Buddy |
| 脏文件页（`PG_dirty`） | writeback 写回，完成后 free |
| 匿名页 | rmap 解映射 → swap out（§5） |
| 仍被引用（`PG_referenced`） | 放回 LRU，给二次机会 |
| mlocked / pin 住 | 跳过（或移 unevictable） |

这就是回收器最核心的分支——**每种页状态对应一个处置动作**，最终目标都是「把物理页安全地还给 Buddy」。

---

## 3. `lru_add_drain()`：per-CPU 攒批

页加入 LRU 不是「每页一次抢锁」，而是**先攒在 per-CPU 缓存里**（`mm/swap.c:163` 的 `lru_add_fn()`），攒够一批再一次性链入 `lruvec->lists[]`。

```c
/* mm/swap.c:163 —— folio 真正链入 LRU 的落点 */
static void lru_add_fn(struct lruvec *lruvec, struct folio *folio)
```

回收扫描前必须 `lru_add_drain()`，把各 CPU 缓存里**还没入链的页先排干**——否则这些页「游离在 LRU 之外」，扫描会漏掉它们。这和 Ch 2 §5 讲的 per-CPU pageset 是同一套「批处理降锁争用」思想。

---

## 4. active → inactive 的降级

`shrink_active_list()`（`vmscan.c:2688`）做反向操作——扫描 active 尾部，把**长期未被引用**的页降级到 inactive：

```
active 尾部页
  ├─ 近期被引用（folio_referenced 真）→ 保留 active（移到链头）
  └─ 长期未引用 → 降到 inactive（成为回收候选）
```

**活跃度判定**走 `folio_referenced()`（rmap 遍历）：它把各进程 PTE 里的 accessed 位**聚合**起来，判断这个页「到底还有没有人在用」。这是 rmap（Ch 3 §7）在回收里的第二个用武之地。

---

## 5. HFT / 嵌入式关联

| 现象 | 本节机制的兑现 |
|------|----------------|
| 回收不卡大锁 | isolate 快进快出，写盘在锁外 |
| 页游离 LRU 之外 | `lru_add_drain()` 先排干 per-CPU 缓存，扫描不遗漏 |
| 工作集保护 | `shrink_folio_list` 里 referenced 页放回，给二次机会 |
| 批量回收 | `SWAP_CLUSTER_MAX` 分批扫描，回收有节奏不阻塞 |

---

## 6. 衔接

- 上节 [§2 页缓存](./section-2-页缓存.md)：file 链上挂的页
- [§4 收缩各类缓存](./section-4-收缩各类缓存.md)：非 LRU 页（slab/dcache）的回收
- [§5 换出进程页面](./section-5-换出进程页面.md)：匿名页 swap out 细节
- 前置：[Ch 2 §5 per-CPU](../../chapter-02-describing-physical-memory/notes/section-5-2.6-内核的新变化.md)（同款批处理思想）

---

<details>
<summary>代码自测（Q&A）</summary>

**Q1：为什么 `shrink_inactive_list` 要把 victim 隔离到临时列表再处理？**
A：为了**锁外做慢活**。`isolate_lru_folios()` 只在持 `lru_lock` 时把页从 LRU 摘下（快操作），然后立刻放锁；后续「写回脏页、swap out」这些可能阻塞甚至等 I/O 的操作，都在**无锁**的临时列表上做。如果边持锁边写盘，其他 CPU 的分配/回收全被堵住。

**Q2：`lru_add_drain()` 为什么要在扫描前调用？**
A：页加入 LRU 是**先攒在 per-CPU 缓存**、后批量入链。如果直接扫描，那些还躺在各 CPU 缓存里、没正式入链的页会被**漏扫**。`lru_add_drain()` 先把它们排干、正式链入 LRU，保证扫描看到的是一致视图。

**Q3：`shrink_folio_list` 回收失败（页放回 LRU）的常见原因有哪些？**
A：① 页还有 `PG_referenced`（近期被访问，给二次机会）；② 页被 pin（`_refcount` 异常，比如 DMA 中）；③ 页正在 writeback（要等 I/O 完成）；④ mlocked（直接跳去 unevictable）。回收器对每类都有专门处理，不是「扫到就硬踢」。

**Q4：active 降级和 inactive 回收怎么配合？**
A：`shrink_active_list` 把长期不用的 active 页降级到 inactive，扩大回收候选池；`shrink_inactive_list` 从 inactive 里实际回收。两者交替驱动，让「工作集」（active）和「回收池」（inactive）动态平衡——这正是 §1 说的「active 容纳工作集、inactive 供回收」。

**Q5：`folio_referenced()` 为什么要走 rmap 遍历 PTE？**
A：因为「页是否被访问」的信息散落在**每个映射它的进程的 PTE accessed 位**里，`struct page/folio` 上没有单一权威位。`folio_referenced()` 通过 rmap 找到所有映射这个 folio 的 PTE，聚合它们的 accessed 位，才能准确判断「到底还有没有人在用」。这就是 rmap 反向映射存在的核心价值之一。

</details>

---
