# Ch 6 §3 页面释放（合并与 v6.6 的尾插启发式）

> **Understanding the Linux Virtual Memory Manager** · Mel Gorman · **选读 🟡**
> 源码核验：Linux **v6.6**（`mm/page_alloc.c` :723 `buddy_merge_likely`、:763 `__free_one_page`、:1183 `free_pcppages_bulk`）

---

## 本节讲什么

释放是分配的镜像：**递归合并（coalesce）** 对称于拆分。但 v6.6 在"挂链表"这一步藏了个反直觉优化——**尾插**（to_tail）。理解它等于理解 buddy 的 cache 行为学，这直接指导用户态 arena 的 free 策略。

---

## 1. 合并主循环（`__free_one_page`，page_alloc.c:763）

```
free 块 (pfn, order)
while (order < MAX_ORDER-1) {
    buddy_pfn = pfn ^ (1 << order);          /* 伙伴 */
    buddy 空闲？(PageBuddy && buddy_order == order)
        否 → break
    是 → 从 free_list 摘掉伙伴
         pfn &= ~(1 << order);  order++;      /* 合成父块，继续向上 */
}
挂入 free_list[order][migratetype]           /* 头插或尾插！见下 */
```

| 性质 | 说明 |
|------|------|
| 合并上限 | 不能超过 pageblock（跨 pageblock 类型可能不同） |
| 全程持 zone->lock | free 的慢路径（order>3 或 pcp 满）才有此开销 |
| 合并的"级联停止" | 伙伴被占即停——中间留洞是常态 |

## 2. v6.6 反直觉优化：尾插（to_tail）

```c
/* page_alloc.c:832（v6.6 实锚） */
to_tail = buddy_merge_likely(pfn, buddy_pfn, page, order);
if (to_tail)
    add_to_free_list_tail(...);   /* 尾插！ */
else
    add_to_free_list(...);        /* 传统头插 */
```

`buddy_merge_likely()`（:723）判断：**这块的更高阶伙伴（祖父块）是否空闲**——若是，释放后马上会被合并走，**挂哪头都活不长**；若否，说明这块大概率要在链上长住。

| 情形 | 插法 | 理由 |
|------|------|------|
| 高阶合并大概率发生 | 尾插 | 反正马上被摘走，别占据头部热位 |
| 块会长住 | 头插 | 下次 `__rmqueue_smallest` 从头取 → **刚释放的热块优先复用**（cache 热） |

**这就是原书 LIFO 热复用思想的 v6.6 精修版**：头部留给"会长住的块"其实是留给"最可能被再取走的热块"。SLUB quicklist → pcp → buddy tail-insert，一条完整的"热块优先"谱系。

## 3. pcp 的批量回吐（`free_pcppages_bulk`，:1183）

order≤3 的 free 先进本 CPU pcp（无 zone 锁）；pcp 计数超过 `high` 水位时：

```
free_pcppages_bulk(zone, count, pcp)
  按 migratetype 轮转挑链 → 批量摘页 → 逐页 __free_one_page（拿 zone 锁一次，合并一批）
```

**批的意义：** zone 锁的获取次数 = free 次数 / batch。batch 由 `high` 水位与 `free_factor`（v6.6 新：free 压力下自动放大 batch）动态决定——**锁次数被压成与流量亚线性**。

## 4. 释放路径全景（对称于 §2 分配瀑布）

```
free_pages(page, order)
  ├─ order ≤ 3 → 本 CPU pcp（无锁快路径）
  │      pcp 超 high → 批量回 buddy（zone 锁）
  └─ order > 3（或需特殊处理）→ 直接 buddy：__free_one_page
                              （zone 锁 + 递归合并）
```

| 路径 | 锁 | 典型耗时 |
|------|----|----------|
| pcp 快路径 | 无（本核） | ~10ns |
| pcp 批量回吐 | zone->lock 一次/batch | 摊薄后仍快 |
| 高阶直入 buddy | zone->lock + 递归合并 | 与合并深度成正比 |

## 5. 原书对照

| 原书（2.4/2.6） | v6.6 |
|------------------|------|
| 释放一律头插 | `buddy_merge_likely` 尾插启发式 |
| pcp 只 order-0 且分 hot/cold | **order 0–3 全走 pcp**，hot/cold 概念已删（cache 控制交给 LRU/compaction） |
| 合并上界 MAX_ORDER | 上界还受 pageblock/migratetype 约束 |
| batch 固定 | `free_factor` 动态缩放 |

## 6. HFT / 嵌入式关联

| 机制 | 用户态镜像 |
|------|-----------|
| 头插热复用 | 池的 free 槽 push 到本核链头（别排队 FIFO——cache 冷） |
| 尾插启发式 | "这块马上会被合并"= 用户态"这个 chunk 马上会被 arena 收割"，不必精细整理 |
| pcp 批量回吐 | 每核池满 → 批量交还全局（一次锁一批） |
| 递归合并级联 | arena 合并相邻 chunk 的 while 循环同构——注意提前停止条件别写错 |

## 7. 观测

```bash
grep -w 'pgfree\|free_pcp\|pcp' /proc/vmstat
# 高阶合并效率：前后对比 pagetypeinfo（free 事件后高阶行 +1 = 合并成功）
```

## 8. 衔接

- [§2 页面分配](./section-2-页面分配.md)：对称的取块
- [§6 pageset](./section-6-2.6-内核的新变化.md)：pcp 细节
- [Ch 8 slab](../../chapter-08-slab-allocator/)：释放的对象级版本（frozen free）

---

<details>
<summary>代码自测（Q&A）</summary>

**Q1：为什么释放后立即看到 order-9 计数没涨？（明明 free 了很多页）**
A：两种可能：① 页进了 pcp 还没回 buddy（高水位未到）；② 合并被占用的伙伴阻断——释放的页散布在已占用块的缝隙里。**碎片化的直接可视化**就是 free 总量涨、高阶计数不涨。

**Q2：头插 vs 尾插，对谁友好？**
A：头插对 **分配者** 友好（热块先出）；尾插对 **合并器** 友好（长住块沉底、短命块排队等合并）。v6.6 用 merge_likely 预测决定——本质是 **按预期寿命分流**，与 LRU 的 young/old 双链同构。

**Q3：释放一个不属于自己的页（double free / 野指针 free）会怎样？**
A：`__free_one_page` 检查有限（PageBuddy 校验伙伴、free 页计数器 sanity），内核态 double free 常见后果是 free_list 成环 → 后续分配返回同一页两次 → 数据腐坏。`CONFIG_DEBUG_VM`/page poisoning 能抓一部分。用户态池同理——句柄化 + free 标志位是最便宜的防线。

**Q4：pcp 的 hot/cold 之分为什么被删了？**
A：实测 free 到 pcp 的页大概率马上被同核再分配（LIFO 已保证热度），冷热分链增加代码路径却收效甚微。5.x 清理删除。**先测量再优化**的反面教材纪念——原书把它当重要机制讲，实测不值得。

**Q5：高阶页（order>3）为什么不能进 pcp？**
A：① 高阶块稀少，per-CPU 缓存命中率低还占计数；② 高阶 free 的意义在 **触发合并**（尽快回 buddy 恢复连续性），压在 pcp 里反而推迟合并；③ pcp 链按 pageblock 内 migratetype 分列，高阶块跨 pageblock 语义复杂。只有高频的 order≤3 值得缓存。

</details>

---
