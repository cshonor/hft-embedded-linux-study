# Ch 10 页框回收 · Page Frame Reclamation

> **Understanding the Linux Virtual Memory Manager** · Mel Gorman · **选读 🟡**

---

## 本章概述

> **Understanding the Linux Virtual Memory Manager** · Mel Gorman · **选读**（HFT：**kswapd / direct reclaim / 脏页回写** 是 **latency 尖刺** 主要来源之一 — 与 **`mlock`、足够 RAM、监控 vmstat** 强相关）

运行一段时间后，物理页框会被 **页缓存、slab、dentry/inode、进程映射页** 等占满。内核必须在 **彻底耗尽前** **挑选旧页回收**，腾出空间给 **新分配**（Ch 6 Buddy）。

> **时代说明：** 原书函数名 **`shrink_cache()`、`refill_inactive()`、`swap_out()`** 等多属 **2.4/2.6**。现代主线为 **`shrink_page_list()` / `shrink_lruvec()`、`balance_pgdat()`** 等（[`mm/vmscan.c`](https://elixir.bootlin.com/linux/latest/source/mm/vmscan.c)）— **active/inactive LRU、kswapd、zone 局部 LRU** 思想不变。

→ 缺页与 swap-in：[Ch 4](../../chapter-04-process-address-space/notes/section-4-异常处理与缺页异常.md#4-异常处理与缺页异常-page-faulting) · rmap 加速解映射：[Ch 3 §7](../../chapter-03-page-table-management/)

---

## 本章在 VM 子系统中的位置

```
Ch 6 alloc_pages 需要 free 页
        ↑
Ch 10 回收：LRU 选 victim → 丢弃 / 写回 / swap out
        ↑
Ch 2 zone 水位 pages_low/min → 唤醒 kswapd 或 direct reclaim
Ch 11 swap：匿名页换出细节
```

**HFT 核心句：** **`GFP_KERNEL` 分配 + 内存紧** → 调用方可能 **同步进 shrink** — **毫秒~秒级 stall**；**`mlock`/`mlockall`** 把 **进程页** 移出 **可换出候选**，但不免疫 **全局 reclaim 对 page cache 的压力**（通常影响较小若 RSS 已钉住）。

---

## 小节笔记

| 节 | 笔记 |
|----|------|
| 1. 页面替换策略 | [notes/section-1-页面替换策略.md](./notes/section-1-页面替换策略.md) |
| 2. 页缓存 | [notes/section-2-页缓存.md](./notes/section-2-页缓存.md) |
| 3. LRU 链表管理 | [notes/section-3-LRU-链表管理.md](./notes/section-3-LRU-链表管理.md) |
| 4. 收缩各类缓存 | [notes/section-4-收缩各类缓存.md](./notes/section-4-收缩各类缓存.md) |
| 5. 换出进程页面 | [notes/section-5-换出进程页面.md](./notes/section-5-换出进程页面.md) |
| 6. 页面换出守护进程 | [notes/section-6-页面换出守护进程.md](./notes/section-6-页面换出守护进程.md) |
| 7. 2.6 内核的新变化 | [notes/section-7-2.6-内核的新变化.md](./notes/section-7-2.6-内核的新变化.md) |

---

## 相关章节

- 上一章：[../chapter-09-high-memory-management/](../chapter-09-high-memory-management/)
- 下一章：[../chapter-11-swap-management/](../chapter-11-swap-management/)
- 附录 J：[../../appendix-J-页框回收.md](../../appendix-J-页框回收.md)
- 全书目录：[OUTLINE.md](../../OUTLINE.md)
