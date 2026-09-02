# 附录 A 简介 · Introduction

> **Code Commentary** · Mel Gorman · **跳过** · 源码核验：Linux v6.6

---

## 本节走读什么

原书 Code Commentary（附录）的定位是**与正文一一对应的源码走读**：正文 Ch1–14 讲「机制为什么这样设计」，附录 A–M 讲「这些机制在 `mm/*.c` 里**代码是怎么组织的**」。

本附录 A 是走读的**总纲**——先建立 `mm/` 目录的全景地图，再说明 A–M 各篇走读哪个文件、用什么方法。

---

## 1. `mm/` 目录全景（v6.6 共 118 个 .c 文件）

按功能归类（完整清单见 `mm/Makefile`）：

| 类别 | 核心文件 | 对应附录 |
|------|----------|:--------:|
| 物理页分配 | `page_alloc.c`、`mm_init.c` | F |
| 非连续分配 | `vmalloc.c` | G |
| Slab/SLUB | `slub.c`、`slab_common.c`、`slab.c` | H |
| 启动分配 | `memblock.c`、`bootmem_info.c` | E |
| 页框回收 | `vmscan.c`、`compaction.c` | J |
| 交换管理 | `swapfile.c`、`swap_state.c`、`page_io.c`、`swap.c`、`swap_slots.c`、`zswap.c` | K |
| OOM | `oom_kill.c` | M |
| 页表/缺页 | `memory.c`、`pgtable-generic.c`、`mmu_gather.c` | C |
| 地址空间 | `mmap.c`、`mlock.c`、`madvise.c`、`gup.c`、`mmap_lock.c` | D |
| 共享内存 | `shmem.c` | L |
| 页缓存 | `filemap.c`、`readahead.c`、`page-writeback.c` | （正文 Ch10） |
| 大页 | `huge_memory.c`、`hugetlb.c`、`khugepaged.c` | （正文 Ch3） |
| 高端内存 | `highmem.c` | I |
| 其他 | `ksm.c`、`memcontrol.c`、`mempolicy.c`、`percpu.c`、`internal.h` | — |

**关键认知**：118 个文件不是平铺的，而是**围绕 `struct page` / `struct folio` 这个中心数据结构**组织起来的——分配（page_alloc/vmalloc/slub）往出拿页，回收（vmscan/compaction/oom）往回收页，页表（memory/pgtable）建立映射，swap/shmem 做页的「换出/共享」外围。

## 2. A–M 走读路线（与正文对应）

```
附录 A  简介（本页：mm/ 全景 + 读法）
附录 B  描述物理内存   → mmzone.h / mm_types.h（struct page/zone/node）   ← Ch2
附录 C  页表管理       → memory.c / pgtable-generic.c / rmap              ← Ch3
附录 D  进程地址空间   → mmap.c / mlock.c / gup.c                         ← Ch4
附录 E  启动内存分配器 → memblock.c                                       ← Ch5
附录 F  物理页分配     → page_alloc.c                                     ← Ch6
附录 G  非连续内存分配 → vmalloc.c                                        ← Ch7
附录 H  Slab 分配器    → slub.c / slab_common.c                           ← Ch8
附录 I  高端内存管理   → highmem.c                                        ← Ch9
附录 J  页框回收       → vmscan.c                                         ← Ch10
附录 K  交换管理       → swapfile.c / swap_state.c / page_io.c            ← Ch11
附录 L  共享内存       → shmem.c                                          ← Ch12
附录 M  内存耗尽管理   → oom_kill.c                                       ← Ch13
```

## 3. 走读方法论（呼应 Ch1 源码路线）

原书 Ch1 给的源码阅读顺序是 `oom_kill.c` → `vmalloc.c` → `page_alloc.c` → `mmap.c`——从「最独立」到「最耦合」。本附录沿用这条原则，走读时**每个文件问四个问题**：

| 问题 | 落点 |
|------|------|
| 这个文件的**核心数据结构**是什么？ | `struct X` 定义 + 字段含义 |
| **入口函数**是哪个？ | `SYSCALL_DEFINE*` / `EXPORT_SYMBOL` 的公开接口 |
| 关键函数**调用链**怎么走？ | A → B → C 的链路 + 每步行号 |
| 与**其他文件**怎么交互？ | 跨文件的 `extern` / 回调 / 数据共享 |

**核对铁律**：所有行号以 `D:\.kernel-ref\` 缓存的 **v6.6 源码**为准（jsdelivr 抓取，已 `wc -c` 验证非 404 残片）。原书是 2.6 时代的代码，**函数名、文件拆分、数据结构都变了**，走读时必须对着 v6.6 重新定位——这 13 篇附录的价值恰恰在「把 2004 的代码注释，重映射到 2026 的真实源码」。

---

## HFT / 嵌入式关联

| 维度 | 落地 |
|------|------|
| 走读优先级 | HFT 先精读 `page_alloc.c`（附录 F，分配延迟）、`vmscan.c`（附录 J，回收尖刺）、`oom_kill.c`（附录 M，防误杀） |
| 读源码目的 | 不是背 API，是**建立「观测值 → 哪个函数 → 哪行代码」的定位链**，延迟抖动时能一路追到源码 |

---

## 相关章节

- 上一章：[./chapter-14-the-final-word/](./chapter-14-the-final-word/)
- 下一章：[appendix-B-描述物理内存.md](./appendix-B-描述物理内存.md)

---

<details>
<summary>自测 5 问（点开看答案）</summary>

**Q1：v6.6 的 `mm/` 目录有多少个 .c 文件？围绕什么中心数据结构组织？**

118 个（`mm/Makefile` 统计），围绕 `struct page` / `struct folio` 组织——分配往外拿页、回收往回收页、页表建立映射、swap/shmem 做外围。

**Q2：原书 Ch1 的源码阅读顺序是什么？为什么这样排？**

`oom_kill.c` → `vmalloc.c` → `page_alloc.c` → `mmap.c`，从「最独立」到「最耦合」——先读依赖少的，逐步深入依赖链。

**Q3：每个文件走读要回答哪四个问题？**

① 核心数据结构是什么；② 入口函数是哪个；③ 关键函数调用链怎么走；④ 与其他文件怎么交互。

**Q4：为什么说这 13 篇附录的价值在「重映射」？**

因为原书是 2.6 时代代码，函数名、文件拆分、数据结构都变了，必须对着 v6.6 重新定位，把「2004 的代码注释」重映射到「2026 的真实源码」。

**Q5：HFT 视角下三个最优先精读的 mm 文件是？**

`page_alloc.c`（附录 F，分配延迟）、`vmscan.c`（附录 J，回收尖刺）、`oom_kill.c`（附录 M，防误杀）。

</details>
