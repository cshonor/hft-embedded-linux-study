# Ch 5 §4 启动内存分配器的退役 (Retiring)

> **Understanding the Linux Virtual Memory Manager** · Mel Gorman · **跳过 ⚪**
> 源码核验：Linux **v6.6**（`mm/memblock.c` 的 `memblock_free_all` / `free_low_memory_core_early`）

---

## 本节讲什么

本节回答一个问题：**memblock 什么时候、怎么把「可用内存」正式交给 Buddy（Ch6 物理页分配器）？**

原书答案是 `mem_init()` 遍历 bootmem 位图移交 Buddy；v6.6 的答案是 **`memblock_free_all()`**——这是启动内存分配器**退役**、常规分配器**接管**的分水岭。

---

## 1. 退役的时机：`start_kernel()` 后期

退役发生在 `start_kernel()` 的 `mm_init()` 阶段，时机有硬约束——**必须等 Buddy + `struct page` 都就绪**：

```
start_kernel()
  └─ mm_init()
       ├─ mem_init()          ← 各 arch：建立直接映射、初始化 struct page
       │     └─ memblock_free_all()   ← 退役！把空闲页交给 Buddy
       ├─ kmem_cache_init()   ← slab 启动（Ch8）
       └─ ...
```

**为什么不能早？** 因为移交 Buddy 需要两样东西：**`struct page` 数组**（每页的元数据，否则 Buddy 无从记账）和**zone 结构**（`free_area` 空闲链表，Ch6）。这两样都是 `mem_init()` 在退役**之前**建好的。

---

## 2. 退役的核心：`memblock_free_all()`

```c
/* mm/memblock.c:2174 */
void __init memblock_free_all(void)
{
    unsigned long pages;

    free_unused_memmap();                 /* ① 释放没用到的 struct page 页 */
    reset_all_zones_managed_pages();      /* ② 清空 zone 的 managed_pages 计数 */

    pages = free_low_memory_core_early(); /* ③ 核心：遍历空闲区间交 Buddy */
    totalram_pages_add(pages);            /* ④ 更新全局 totalram_pages */
}
```

| 步骤 | 做什么 |
|------|--------|
| ① `free_unused_memmap()` | 有些 `struct page` 页是**空洞/保留区**的，根本没对应物理页，把这段 memmap 也释放 |
| ② `reset_all_zones_managed_pages()` | 把各 zone 的 `managed_pages` 归零，准备重新统计 |
| ③ `free_low_memory_core_early()` | **真正移交**：遍历 memblock 的空闲区间，逐页挂进 Buddy |
| ④ `totalram_pages_add()` | 累加全局可用页计数 |

---

## 3. 移交细节：`free_low_memory_core_early()`

```c
/* mm/memblock.c:2126 */
static unsigned long __init free_low_memory_core_early(void)
{
    unsigned long count = 0;
    phys_addr_t start, end;
    u64 i;

    memblock_clear_hotplug(0, -1);
    memmap_init_reserved_pages();          /* 初始化 reserved 区的 struct page */

    for_each_free_mem_range(i, NUMA_NO_NODE, MEMBLOCK_NONE,
                            &start, &end, NULL)   /* 遍历 memory − reserved 的空闲区间 */
        count += __free_memory_core(start, end);  /* 逐段按 order 拆成页交 Buddy */

    return count;
}
```

**关键动作**：

```
memblock 空闲区间（memory − reserved）
   [start ───────────────────── end)
        │  __free_memory_core()
        │  按 MAX_ORDER 对齐，把整段拆成 2^n 页块
        ▼
   __free_pages_bootmem() → memblock_free_pages()
        │  page 已有 struct page，可进 Buddy
        ▼
   Buddy freelist（ZONE_NORMAL / ZONE_DMA32 … 的 free_area）
```

**与 §3「假释放」的呼应**：到这里 `struct page` 已经建好，`memblock_free_pages()` 能把每页真正挂进 Buddy 的 `free_area` 链表——**「free」从记账变成现实**。之前 `memblock_free()` 做不到，就是因为缺 `struct page`。

---

## 4. 退役之后：memblock 去哪了

| 场景 | 行为 |
|------|------|
| 默认 | `memblock_discard()` 把 `memblock` 的静态数组**释放**，`__init` 段的内存也回收 |
| `CONFIG_ARCH_KEEP_MEMBLOCK` | 保留 memblock，供**调试**（`/sys/kernel/debug/memblock` 导出两表） |
| 退役后还有少量分配需求 | `memblock_free_late()`（`memblock.c:1658`）直接把页还 Buddy（绕过 memblock 记账） |

**之后**：`kmalloc`、`__alloc_pages`、`mmap` fault 等**全部走 Buddy/slab**，用户态进程**永远不会**触达 memblock 路径——它只活在启动的几百毫秒里。

---

## 5. HFT / 嵌入式关联

| 现象 | 本节机制的兑现 |
|------|----------------|
| 系统起来后 `free` 显示的可用内存 | 就是 `memblock_free_all()` 交给 Buddy 的「memory − reserved」总量 |
| 启动快慢（嵌入式关键指标） | 退役时 `for_each_free_mem_range` 的遍历 + 逐页挂链是启动耗时一部分 |
| 排查「内存没全进 Buddy」 | 看 `memblock=debug` + `dmesg` 的 `Memory:` 行，与物理总量对账 |

---

## 6. 衔接

- 下一章：[Ch6 物理页分配](../../chapter-06-physical-page-allocation/)（Buddy 接管后如何服务 `__alloc_pages`）
- 节点/zone 结构：[Ch2 内存节点](../../chapter-02-describing-physical-memory/notes/section-1-内存节点.md)（退役时 zone 就绪的前提）

---

<details>
<summary>代码自测（Q&A）</summary>

**Q1：为什么 `memblock_free_all()` 必须等 `struct page` 建好才能执行？**
A：因为移交 Buddy 的本质是「把物理页挂进 zone 的 `free_area` 空闲链表」，而 `free_area` 里放的是**指向 `struct page` 的指针**。没有 `struct page` 数组，就没有 per-page 记账对象，Buddy 无法追踪「哪些页空闲、哪些页组成了 2^n 块」。所以 `mem_init()` 要先建 `struct page`，再调 `memblock_free_all()`。

**Q2：`free_unused_memmap()` 释放的是什么「没用到的 memmap」？**
A：`struct page` 数组（memmap）是按**物理地址跨度**分配的，但有些地址是**空洞/保留区**（物理内存没插满、firmware 保留），对应的 `struct page` 永远用不到。`free_unused_memmap()` 把这些「白占内存的 `struct page` 页」释放掉，节省内存。

**Q3：`for_each_free_mem_range` 遍历的「空闲区间」是怎么算出来的？**
A：它就是 **`memblock.memory` 减去 `memblock.reserved`** 的差集。memblock 内部按地址顺序同时遍历两表，找出「在 memory 里、但不在 reserved 里」的连续段。这正是 §1「两表模型」的最终消费方式——**分配和退役都用同一个差集**。

**Q4：`memblock_free_all` 之后还能用 `memblock_alloc` 吗？**
A：默认不能——`memblock_discard()` 把 memblock 的静态存储释放了，`memblock_alloc` 内部依赖的 `memory`/`reserved` 表已不存在。这也是为什么 memblock 的 API 都带 `__init`/`__init_memblock` 标记：它们只活在启动期，退役后其代码段本身也随 `__init` 一起被回收。

**Q5：`reset_all_zones_managed_pages()` 为什么要把 zone 计数归零再统计？**
A：因为 zone 的 `managed_pages`（受 Buddy 管理的页数）在退役**之前**可能已经被 `free_area_init` 预填过一遍，退役时用 `memblock_free_all` 重新、**权威地**统计一次。归零是「防止旧值累加导致 double count」——先清空，再按 memblock 的实际空闲区间重算，保证 `managed_pages` 精确。

</details>

---
