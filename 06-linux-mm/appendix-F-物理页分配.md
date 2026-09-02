# 附录 F 物理页分配 · Physical Page Allocation

> **Code Commentary** · Mel Gorman · **选读** · 源码核验：Linux v6.6

概念总览 → [./chapter-06-physical-page-allocation/](./chapter-06-physical-page-allocation/)

---

## 本节走读什么

正文 Ch6 讲了「Buddy 分配器、GFP、水位」。本附录走读 **`mm/page_alloc.c`（189KB，mm/ 里最大文件）** 的两条核心路径：**分配**（`__alloc_pages` fast/slow path）与**释放**（`free_unref_page` → Buddy 合并）。

---

## 1. 分配路径全景（fast → slow）

```
__alloc_pages(gfp, order, nid)                    // page_alloc.c:4390  入口
        │
        ▼
get_page_from_freelist(...)                       // page_alloc.c:3048  fast path
        │  遍历 zonelist，从 free_area 直接拿（关抢占 + per-CPU）
        ├─ 成功 → 返回
        │  失败（低于水位 / 没连续页）↓
        ▼
__alloc_pages_slowpath(...)                       // page_alloc.c:3899  slow path
        ├─ __alloc_pages_direct_reclaim(...)      // :3640  直接回收（Ch10）
        ├─ __alloc_pages_direct_compact(...)      // :3368  内存压缩
        ├─ should_reclaim_retry(...)              // :3794  回收后重试判断
        └─ __alloc_pages_may_oom(...)             // :3273  OOM 兜底（Ch13）
```

**走读要点**：fast path（`get_page_from_freelist`）**尽量不开锁、不回收**，从 per-CPU 缓存（`pcp`）或 `free_area` 直接拿；只有 fast path 失败才进 slow path，依次试「回收 → 压缩 → 重试 → OOM」。这解释了为什么「分配延迟」在正常情况下是纳秒级（fast path 命中），一旦进入 slow path 就变成毫秒级尖刺。

## 2. Buddy 拿页：`__rmqueue_smallest`（:1565）

从 `free_area[order]` 链表取一个空闲块，若 order 不够则**向上拆分**：

```
__rmqueue_smallest(zone, order, migratetype)      // :1565
        │  从 free_area[order] 取头
        │  若 free_area[order] 空，向上找更大的 order
        ▼
expand(...)                                        // 把大块拆成小块，回填低 order 链表
```

**走读要点**：这是 Buddy「分裂」的实现——要 order-0 页但只有 order-3 空闲块时，把 order-3 拆成 8 个 order-0，取 1 个、其余 7 个挂回链表。

## 3. 释放路径：`free_unref_page` → 合并（:2397）

```c
void free_unref_page(struct page *page, unsigned int order)   // :2397
{
    ... free_unref_page_prepare(page, pfn, order);            // :2307 校验
    free_unref_page_commit(zone, pcp, page, migratetype, order);  // :2367 先入 per-CPU
        │  pcp 满了才 flush 到 Buddy
        ▼
__free_one_page(page, pfn, zone, order, migratetype, ...)    // :763  Buddy 合并
        │  与「伙伴页」检查是否都空闲 → 合并升 order，循环
```

**走读要点**：释放**先入 per-CPU 缓存**（`pcp`），攒够一批才 flush 回 Buddy，减少锁竞争（Ch2 §5 的 `per_cpu_pages`）。真正的 Buddy 合并（`__free_one_page`）只在 flush 时发生——与相邻空闲伙伴页**递归合并升 order**。

## 4. 三条回退路径的定位

| 函数 | 行号 | 触发条件 |
|------|------|----------|
| `__alloc_pages_direct_reclaim` | :3640 | 水位过低，进程自己回收（Ch10 direct reclaim） |
| `__alloc_pages_direct_compact` | :3368 | 高阶分配碎片化，压缩出连续页 |
| `__alloc_pages_may_oom` | :3273 | 前两者都失败，走 OOM（Ch13） |

**走读要点**：这三条路径都在 `__alloc_pages_slowpath` 里**按顺序**尝试，且受 `gfp_mask` 控制（`__GFP_DIRECT_RECLAIM`/`__GFP_KSWAPD_RECLAIM` 决定是否允许）。HFT 里「一次分配触发了 direct reclaim」的延迟尖刺，就源自 :3640 这条路径。

---

## 与正文对应

| 附录内容 | 正文落点 |
|----------|----------|
| fast/slow path 两段 | Ch6（分配流程） |
| `__rmqueue_smallest` 分裂 | Ch6（Buddy split） |
| `__free_one_page` 合并 | Ch6（Buddy merge） |
| direct reclaim/compact/OOM | Ch10/Ch13（回收/OOM） |

---

## HFT / 嵌入式关联

| 手段 | 落点 |
|------|------|
| 观察分配路径 | `perf probe __alloc_pages_slowpath` 统计「多少次分配进了 slow path」，是延迟尖刺的直接指标 |
| 避免高阶分配 | order>0 的分配更容易碎片化触发 compact，HFT 尽量用 order-0（4K） |
| 理解 pcp | per-CPU 缓存让「释放后再分配」几乎零锁，但跨 CPU 迁移会失效 |

---

## 相关章节

- 上一章：[appendix-E-启动内存分配器.md](./appendix-E-启动内存分配器.md)
- 下一章：[appendix-G-非连续内存分配.md](./appendix-G-非连续内存分配.md)

---

<details>
<summary>自测 5 问（点开看答案）</summary>

**Q1：`__alloc_pages` 的 fast path 和 slow path 分别是哪个函数？**

fast path 是 `get_page_from_freelist`（:3048），slow path 是 `__alloc_pages_slowpath`（:3899）。

**Q2：fast path 为什么快？**

尽量不开锁、不回收，从 per-CPU 缓存（pcp）或 `free_area` 直接拿；只有失败才进 slow path 试回收/压缩/OOM。

**Q3：`__rmqueue_smallest` 怎么处理「order 不够」？**

从 `free_area[order]` 取头；若该 order 为空，向上找更大的空闲块，用 `expand` 拆成小块、取一个、其余回填低 order 链表。

**Q4：释放页为什么先入 per-CPU 缓存而不是直接回 Buddy？**

减少锁竞争（攒批 flush）；真正的 Buddy 合并（`__free_one_page`，:763）只在 flush 时发生，与空闲伙伴页递归合并升 order。

**Q5：HFT 里「一次分配触发 direct reclaim」的延迟尖刺源自哪条路径？**

`__alloc_pages_direct_reclaim`（:3640）——slow path 里进程自己回收内存，是毫秒级延迟尖刺的直接来源。

</details>
