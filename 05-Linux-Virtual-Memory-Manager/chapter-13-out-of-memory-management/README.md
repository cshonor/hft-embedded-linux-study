# Ch 13 内存耗尽管理 · Out of Memory Management

> **Understanding the Linux Virtual Memory Manager** · Mel Gorman · **跳过 ⚪**

---

## 本章概述

> **Understanding the Linux Virtual Memory Manager** · Mel Gorman · **跳过**（HFT：**生产 latency 机** 应 **never 触发 OOM** — 足够 RAM + **`mlock`** + 监控；本章解释 **谁会被杀、为何突然 SIGKILL**）

原书 **故意写短** — OOM 管理器任务单一：

```
有足够内存？ → 否且回收已尽力 → 真 OOM？ → 选进程 → 杀死 → 释放内存
```

> **源码入口（Ch 1 阅读路线第 1 步）：** [`mm/oom_kill.c`](https://elixir.bootlin.com/linux/latest/source/mm/oom_kill.c) — 牵涉 **zone 水位、回收、swap、mm_struct** 等多处，但 **控制流相对集中**，适合 **第一次读 `mm/`**。

> **现代扩展：** **cgroup v1/v2 memory** 的 **memcg OOM**、**`oom_score_adj`**、**pid 1 保护** 等 — 原书 **`badness()`** 思路仍在，细节以当前树为准。

→ 回收失败背景：[Ch 10](../../chapter-10-page-frame-reclamation/) · **`PF_MEMDIE`**：[Ch 6](../../chapter-06-physical-page-allocation/notes/section-4-GFP-标志与进程标志.md#4-gfp-标志与进程标志-gfp--process-flags) · **`VM_ACCOUNT`**：[Ch 12](../../chapter-12-shared-memory-virtual-filesystem/notes/section-5-2.6-内核的新变化.md#5-26-内核的新变化)

---

## 本章在 VM 子系统中的位置

```
Ch 6 alloc 失败 / Ch 10 shrink 扫尽仍不够
        ↓
Ch 13 out_of_memory() → select → kill
        ↓
物理页、swap cache、页表、struct page 随进程退出归还
```

**HFT：** OOM killer **按 badness 选「大内存、年轻进程」** — 未 **`mlock`** 的 **大 heap 交易进程** 可能是 **牺牲品**；与 **策略无关**，纯 **内核启发式**。

---

## 小节笔记

| 节 | 笔记 |
|----|------|
| 1. 检查可用内存 | [notes/section-1-检查可用内存.md](./notes/section-1-检查可用内存.md) |
| 2. 确定 OOM 状态 | [notes/section-2-确定-OOM-状态.md](./notes/section-2-确定-OOM-状态.md) |
| 3. 选择要杀死的进程 | [notes/section-3-选择要杀死的进程.md](./notes/section-3-选择要杀死的进程.md) |
| 4. 杀死选定的进程 | [notes/section-4-杀死选定的进程.md](./notes/section-4-杀死选定的进程.md) |
| 5. 2.6 内核的新变化 | [notes/section-5-2.6-内核的新变化.md](./notes/section-5-2.6-内核的新变化.md) |

---

## 相关章节

- 上一章：[../chapter-12-shared-memory-virtual-filesystem/](../chapter-12-shared-memory-virtual-filesystem/)
- 下一章：[../chapter-14-the-final-word/](../chapter-14-the-final-word/)
- 附录 M：[../../appendix-M-内存耗尽管理.md](../../appendix-M-内存耗尽管理.md)
- 全书目录：[OUTLINE.md](../../OUTLINE.md)
