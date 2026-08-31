# Ch 3 §3 页表的分配与释放（Quicklists → ptdesc）

> **Understanding the Linux Virtual Memory Manager** · Mel Gorman · **精读 🔴**
> 源码核验：Linux **v6.6**（`include/asm-generic/pgalloc.h`、`mm/memory.c`、`mm/mmu_gather.c`）

---

## 本节讲什么

页表 **本身就是物理页**，也要从分配器来。原书讲 2.4 的 quicklist 专用缓存；v6.6 的答案是 **`pgtable_t` + ptdesc + kmem_cache**——但"别让页表分配掉进慢路径"这个目标二十年没变。本节把两代机制对照清楚。

---

## 1. 原书机制：Quicklists（2.4 时代）

| Quicklist | 缓存什么 | 特性 |
|-----------|----------|------|
| `pgd_quicklist` | PGD 表页 | per-CPU LIFO |
| `pmd_quicklist` | PMD 表页 | 关中断操作（无锁） |
| `pte_quicklist` | PTE 表页 | 容量上限（否则内存泄漏倾向） |

**动机：** fork+exit 高频的服务器（cgi 时代）页表页反复 alloc/free，buddy 路径（zone lock、伙伴合并）太贵。LIFO 保证 **cache 热** 的表页优先复用。

**问题：** 各 arch 私有实现、无水印控制（quicklist 不算 free memory，OOM 判断失真）——**2.6 即被删除**。

## 2. v6.6 机制：`pgtable_t` + ptdesc

```c
/* include/asm-generic/pgalloc.h（实锚） */
pte_t *__pte_alloc_one_kernel(struct mm_struct *mm);       /* :19 内核 PTE（init_mm 用） */
pgtable_t __pte_alloc_one(struct mm_struct *mm, gfp_t gfp) /* :64 用户 PTE，跑 pagetable_pte_ctor() */
```

| 组件 | v6.6 角色 |
|------|-----------|
| `pgtable_t` | `struct page *` 别名——**一页表 = 一整页**，直接走 buddy（`__get_free_page`）/slab PGD cache |
| `ptdesc`（`struct ptdesc`） | 页表页的元数据头（复用 `struct page` 内存布局），v6.6 正在把"页表页的 page"与"数据页的 page"区分开 |
| `pagetable_pte_ctor()` | 构造：挂到 `mm->pte_table_dtor` 可跟踪、`inc_lru_vec_page_state(PGTABLE)` 计数 |
| PGD | `kmem_cache_alloc(pgd_cache)`（x86 用专用 cache，因 PGD 只占半页要**共享缓存行**） |

**分配时机——锁外预分配**（v6.6 memory.c 实锚）：

```c
/* memory.c:4202 / :4280 / :4520（缺页路径） */
vmf->prealloc_pte = pte_alloc_one(vmf->vma->vm_mm);   /* 拿 mmap_lock 前先分配 */
...
pte_free(vmf->vma->vm_mm, vmf->prealloc_pte);         /* 用不上则退还 */
```

缺页 handler 在 **进入临界区前** 预备好表页——如果直接在锁内 `__GFP_DIRECT_RECLAIM` 分配，可能睡眠持锁，拖垮整个 mm 的并发缺页。**"分配放锁外"是 v6.6 页表路径的硬纪律**，quicklist 时代精神的现代变体。

**copy 过程（fork 建子进程页表）实锚：**

```c
/* memory.c:437  copy_pte_range() */
pgtable_t new = pte_alloc_one(mm);      /* 先备好子进程的表页 */
/* :449  init_mm 的内核表用 pte_alloc_one_kernel() */
```

## 3. 释放：为什么不能"立刻 free"

释放页表页的难点：**别的 CPU 的 TLB 里可能还缓存着指向它的翻译**。TLB 是**异步失效**的（发 IPI 等对方 flush），如果立刻把表页还给 buddy 并被改写，别的核再 walk 会读到 **任意脏数据**。

v6.6 解法 = **mmu_gather 批量延迟释放**（`mm/mmu_gather.c`）：

```
tlb_gather_mmu(&tlb, mm, start, end)     ┐
  unmap_page_range(&tlb, ...)            │ ① 先只把待 free 的表页挂进
    free_pgd_range(&tlb, ...)            │    tlb->local + batch 链表
    tlb_remove_table(tlb, table)         ┘    （mmu_gather.c:18 tlb_next_batch 链式批）
tlb_finish_mmu(&tlb)                      → ② 先 flush TLB（发 IPI 等确认）
                                           ③ 之后才真正 free 表页
```

| mmu_gather 事实（v6.6 实锚） | 值 |
|------------------------------|----|
| 批链表增长 | `tlb_next_batch()`（mmu_gather.c:18），新批 `__get_free_page(GFP_NOWAIT|__GFP_NOWARN)`（:35） |
| 批数上限 | `MAX_GATHER_BATCH_COUNT`（防 unmap 大区间把内存吃爆，:32） |
| RCU 释放 | `CONFIG_MMU_GATHER_RCU_TABLE_FREE`：表页经 RCU grace period 后释放——正在 rcu 读侧 walk 的 `pte_offset_map` 不会踩空 |

**这就是 munmap 大区间时的三段式：解引用 → flush → free。** "unmap 完 TLB 一定干净"在多核上 **必须等 IPI 往返**，不免费。

## 4. 两代机制对照

| | 2.4 Quicklist | v6.6 ptdesc+mmu_gather |
|---|---------------|------------------------|
| 缓存对象 | 表页裸页 | PGD 走 slab cache；PTE/PMD/PUD 整页走 buddy |
| 并发正确性 | 关中断躲 | mmap_lock + RCU + batched flush |
| 可观测性 | 无 | `cat /proc/vmstat \| grep pgtable`（PGTABLE_PTE_ALLOC 等计数） |
| 设计遗产 | LIFO 热复用 | 保留在 slab/pcp（per-CPU page list）里——**quicklist 的魂进了 pcp** |

## 5. HFT / 嵌入式关联

| 现象 | 机制兑现 |
|------|----------|
| fork 风暴后 RSS 虚高 | 每子进程 PGD+PUD+PMD+PTE 4 页/2MiB 映射；页表本身吃内存（`/proc/vmstat` pgtable 计数可证） |
| TLB shootdown 尖刺 | 大批 `zap`→ ①②③ 模型里的 ②：IPI 等待是同步点 |
| ARM64 上 pgtable 计数 | 每表页 ctor 后在 `/proc/vmstat` 可见，量价页表内存 |
| 想清空 page cache 页表 | `munlock`/`madvise(DONTNEED)` 走同一 mmu_gather 路径 |

## 6. 衔接

- [§6 TLB 与 L1 Cache 管理](./section-6-TLB-与-L1-Cache-管理.md)：② 步 flush 的语义
- [Ch 6 物理页分配](../../chapter-06-physical-page-allocation/)：表页的底层来源（buddy/pcp）
- [Ch 8 slab](../../chapter-08-slab-allocator/)：PGD cache 的底层
- 现代全貌：[06.5/ch04](../../../06.5-modern-mm/chapter-04-page-table-tlb/)

---

<details>
<summary>代码自测（Q&A）</summary>

**Q1：为什么 PTE 表页不能像普通数据页一样"解除映射后立即 free"？**
A：TLB 异步失效。别的核可能仍持旧翻译，且页表 walker 硬件不走 TLB 的路径可能引用表页。必须先收集→flush→（RCU grace）→free，即 mmu_gather 三段式。立即 free 的后果 = 野指针式页表读取，随机崩溃。

**Q2：`pgtable_t` 是什么类型？**
A：`typedef struct page *pgtable_t`——指向整页的指针。页表页 **永远整页分配**（512 项×8B=4KiB 恰好一页），所以不需要子页分配器，buddy 直接可服务；元数据寄生在 `struct page`（v6.6 抽象为 ptdesc）。

**Q3：quicklist 删除后，"热表页复用"由谁继承？**
A：两条路：PGD → slab cache（对象大小即半页/一页，slab 天然缓存热对象）；PTE/PMD → buddy 的 per-CPU pageset（pcp）——free 到本 CPU pcp、alloc 优先从本 CPU pcp 拿，等效 LIFO 热复用且受 watermark 管辖。

**Q4：fork 时子进程页表页从哪来？是复制父进程表页本身吗？**
A：`copy_pte_range` 先 `pte_alloc_one` 新页（memory.c:437），再逐项拷 PTE 值。**表页不共享**（COW 的是数据页，不是表页）；表页项倒是指向同样的物理页（只读+refcount++）。

**Q5：`mmu_gather` 的 batch 为什么用 `GFP_NOWAIT` 分配？**
A：`tlb_next_batch` 在 unmap 路径深处（可能持 mmap_lock 写锁），NOWAIT 保证 **不会 reclaim/睡眠**——分配失败就先 flush 当前批腾位子。锁内不可睡眠纪律的又一实例。

</details>

---
