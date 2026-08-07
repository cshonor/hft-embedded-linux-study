# Mel Gorman 内存管理书过时评估 + 现代映射

> 本文评估 `09-linux-mm`（Mel Gorman《Understanding the Linux Virtual Memory Manager》）的过时程度，
> 并给出笨叔《奔跑吧 Linux 内核》卷1 + LWN 文章的替代映射。

---

## 一、过时程度评估

| 主题 | Mel Gorman 书中讲的 (2.4/2.6) | 现代内核 (6.x) | 过时程度 |
|------|-------------------------------|----------------|----------|
| 物理内存描述 | zone 结构、wait_queues | zone 仍在但内部重构、pcp_lists | 中度过时 |
| 页表管理 | 3/4 级页表 | 5 级页表 (PGD→P4D→PUD→PMD→PTE) | 中度 |
| Boot 内存分配 | bootmem 分配器 | memblock 取代 bootmem (已删除) | **严重** |
| 物理页分配 | 伙伴系统、per-cpu pages | 伙伴系统仍在、pcp 重构、folio API | 中度 |
| 非连续内存 | vmalloc、vmap | vmalloc 仍在、vmap_atoms → vm_map_ram | 轻度 |
| Slab 分配器 | SLAB（主）、SLOB/SLUB（备） | SLUB 默认、SLAB 已移除 (6.1+)、SLOB 已移除 (6.4+) | **严重** |
| 高端内存 | highmem zone、kmap | ARM64 无 highmem、x86 逐步弱化 | **严重** |
| 页回收 | LRU 链表、inactive/active | MGLRU 多代 LRU (6.1+) | **严重** |
| Swap | swap cache、swapfile | 仍在但重构、zswap、zram | 中度 |
| OOM | OOM killer | OOM killer 重构、cgroup OOM、PSI | 中度 |

---

## 二、笨叔《奔跑吧 Linux 内核》卷1 — 内存管理对应表

> 笨叔卷1 基于 Linux 5.x，是 Mel Gorman 书的现代中文替代品。

| 笨叔卷1 章 | 标题 | 对应 Mel Gorman 章 | 精读? |
|-----------|------|-------------------|-------|
| Ch3 | 内存管理之预备知识 | Ch1-2 (概述 + 物理内存) | **精读** |
| Ch4 | 物理内存与虚拟内存 | Ch3+6 (页表 + 页分配) | **精读** |
| Ch5 | 内存管理之高级主题 | Ch7+8+9 (vmalloc + slab + highmem) | **精读** |
| Ch6 | 内存管理之实战案例 | — (Mel Gorman 无案例) | **精读** |

### 笨叔卷1 Ch3-6 详细子节 → Mel Gorman 映射

| 笨叔子节 | 内容 | Mel Gorman 对应 | 现代变化 |
|----------|------|----------------|----------|
| Ch3: UMA/NUMA | 内存架构 | Ch2.1 | NUMA 仍在，API 重构 |
| Ch3: zone 初始化 | pg_data_t、zone | Ch2.2 | zone 结构大幅修改 |
| Ch3: 物理内存初始化 | memblock | Ch5 (bootmem) | **bootmem → memblock** |
| Ch4: 页面分配快速路径 | alloc_pages、伙伴系统 | Ch6 | folio API (5.16+) |
| Ch4: 分配掩码 | GFP_MASK | Ch6.3 | 掩码重新整理 |
| Ch4: 伙伴系统核心 | __alloc_pages | Ch6.2 | 重构、watermark 改进 |
| Ch4: 释放页面 | __free_pages | Ch6.4 | folio_release |
| Ch4: 页表 | PGD→PTE | Ch3 | **3/4 级 → 5 级页表** |
| Ch5: SLUB 分配器 | kmem_cache、slab | Ch8 (SLAB) | **SLAB 已移除，SLUB 默认** |
| Ch5: vmalloc | 非连续内存 | Ch7 | vmalloc 重构 |
| Ch5: 内核映射 | kmap、kmap_atomic | Ch9 (highmem) | ARM64 无 highmem |
| Ch5: per-cpu | pcp_pages | Ch6.5 | pcp 重构 |

---

## 三、现代内存管理 LWN 文章精选

> 以下文章覆盖 Mel Gorman 书中已过时的关键主题。

### Slab/SLUB

| 主题 | LWN 文章 | 年份 |
|------|---------|------|
| SLUB 简介 | [SLUB: The unqueued slab allocator](https://lwn.net/Articles/229096/) | 2007 |
| SLUB vs SLAB 性能 | [SLUB performance](https://lwn.net/Articles/229160/) | 2007 |
| SLAB 终被移除 | [Removing the SLAB allocator](https://lwn.net/Articles/949862/) | 2023 |

### 页回收 / MGLRU

| 主题 | LWN 文章 | 年份 |
|------|---------|------|
| LRU 基础 | [Page replacement in Linux](https://lwn.net/Articles/845171/) | 2021 |
| MGLRU 简介 | [Multi-gen LRU](https://lwn.net/Articles/856831/) | 2022 |
| MGLRU 合入主线 | [MGLRU merged for 6.1](https://lwn.net/Articles/913685/) | 2022 |

### Folio / page API

| 主题 | LWN 文章 | 年份 |
|------|---------|------|
| Folio 提案 | [Folios and the page-cache API](https://lwn.net/Articles/849438/) | 2021 |
| Folio 深入 | [The folio API](https://lwn.net/Articles/862108/) | 2021 |
| Folio 合入 | [Folio status](https://lwn.net/Articles/893852/) | 2022 |

### 其他现代内存主题

| 主题 | LWN 文章 | 年份 |
|------|---------|------|
| memblock | [The memblock allocator](https://lwn.net/Articles/449283/) | 2011 |
| 5 级页表 | [Five-level page tables](https://lwn.net/Articles/717293/) | 2017 |
| psi (压力信息) | [PSI: pressure stall information](https://lwn.net/Articles/759781/) | 2018 |
| zswap | [Compressing memory with zswap](https://lwn.net/Articles/537422/) | 2013 |
| DAMON | [DAMON: Data access monitoring](https://lwn.net/Articles/812704/) | 2020 |

---

## 四、推荐学习路线

```
Mel Gorman 过时章节 → 笨叔卷1 → LWN 文章 → 内核源码 mm/

1. 物理内存: 09 Ch2 → 笨叔 Ch3 → mm/mm_init.c, mm/page_alloc.c
2. 页表:    09 Ch3 → 笨叔 Ch4 → mm/pgtable*.c, arch/arm64/include/asm/pgtable.h
3. 伙伴系统: 09 Ch6 → 笨叔 Ch4 → mm/page_alloc.c
4. SLUB:    09 Ch8 → 笨叔 Ch5 → mm/slub.c (注意: SLAB mm/slab.c 已删除)
5. 页回收:  09 Ch10 → 笨叔 Ch5 → mm/vmscan.c (MGLRU: mm/vmscan.c multigen)
6. OOM:     09 Ch13 → LWN PSI → mm/oom_kill.c
```
