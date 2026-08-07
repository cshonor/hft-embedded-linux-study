# Ch 7 进程调度 · Process Scheduling

> **Understanding the Linux Kernel** 3rd · Bovet & Cesati · **🔴 HFT 精读**  
> Linux 2.6 **O(1) 调度器** — 何时切换、选谁运行

---

## ⚠️ 过时标记（ULK3 基于 Linux 2.6，现为 6.x）

| ULK3 讲的 | 现代变化 | 替代资料 |
|-----------|---------|----------|
| **O(1) 调度器** | 2.6.23 起 **CFS** 取代；6.6 起 **EEVDF** 取代 CFS | [CFS scheduling](https://lwn.net/Articles/230501/) (2007) |
| 优先级数组 + 时间片 | vruntime + 红黑树（CFS）；EEVDF 用虚拟截止时间 | [EEVDF Scheduler](https://lwn.net/Articles/969062/) (2024) |
| `recalc_task_prio()` | **已删除** | [What is EEVDF?](https://lwn.net/Articles/927168/) |
| `runqueue` 结构 | `cfs_rq` → `eevdf_rq`，数据结构重写 | [The earliest eligible virtual deadline first](https://lwn.net/Articles/925371/) |

> **原则**：ULK3 的 O(1) 调度器已完全过时。CFS（2.6.23-6.5）和 EEVDF（6.6+）是两代全新设计。务必读 LWN EEVDF 系列文章。

---

## 小节笔记

| 节 | 笔记 |
|----|------|
| 1. 本章定位 | [notes/section-1-本章定位.md](./notes/section-1-本章定位.md) |
| 2. 调度策略与抢占 | [notes/section-2-调度策略与抢占.md](./notes/section-2-调度策略与抢占.md) |
| 3. 调度器数据结构 | [notes/section-3-调度器数据结构.md](./notes/section-3-调度器数据结构.md) |
| 4. 调度算法与核心函数 | [notes/section-4-调度算法与核心函数.md](./notes/section-4-调度算法与核心函数.md) |
| 5. SMP 运行队列平衡 | [notes/section-5-SMP运行队列平衡.md](./notes/section-5-SMP运行队列平衡.md) |
| 6. 调度相关系统调用 | [notes/section-6-调度相关系统调用.md](./notes/section-6-调度相关系统调用.md) |

---

## 相关

- 上一章：[chapter-06-timing/](../chapter-06-timing/)
- 下一章：[chapter-08-memory-management/](../chapter-08-memory-management/)
- 衔接：[chapter-03-processes/](../chapter-03-processes/) · [chapter-05-kernel-synchronization/](../chapter-05-kernel-synchronization/) · [chapter-10-system-calls.md](../chapter-10-system-calls.md)
- [OUTLINE.md](../OUTLINE.md) · [LEARNING_PLAN.md](../LEARNING_PLAN.md)
