# Ch 3 §1 页目录与页表项 (PGD / P4D / PUD / PMD / PTE)

> **Understanding the Linux Virtual Memory Manager** · Mel Gorman · **精读 🔴**
> 源码核验：Linux **v6.6**（`include/linux/pgtable.h`、`include/linux/mm_types.h`）

---

## 本节讲什么

页表是 **VA → PA 的硬件查表数据结构**。本节回答三个问题：

1. 页表分几级？为什么是这么多级？
2. 每一级的表项（entry）长什么样、放哪些位？
3. 内核用什么 **类型系统** 把它抽象成架构无关代码？

原书以 2.4/2.6 的 **三级模型**（PGD→PMD→PTE，x86-32 语境）叙述；v6.6 的真实结构是 **可配置层级**，由 `CONFIG_PGTABLE_LEVELS` 决定——x86_64/ARM64 均为 **四级 + 一个折叠的 P4D**。

---

## 1. 层级结构：从三级到五级

| 层级 | 全称 | v6.6 索引宏（pgtable.h） | x86_64 位宽（48-bit VA） |
|------|------|--------------------------|--------------------------|
| PGD | Page Global Directory | `pgd_offset(mm, addr)`（:145） | 9 |
| P4D | Page 4th Level | `p4d_offset(pgd, addr)` | **0（折叠）** |
| PUD | Page Upper Directory | `pud_offset(p4d, addr)`（:129） | 9 |
| PMD | Page Middle Directory | `pmd_offset(pud, addr)`（:121） | 9 |
| PTE | Page Table Entry | `pte_offset_map(...)` | 9 + 12 位页内偏移 |

```
47        38 37     30 29     21 20     12 11         0
┌───────────┬──────────┬──────────┬──────────┬──────────┐
│ PGD index │ PUD index│ PMD index│ PTE index│ 页内偏移  │
└─────┬─────┴────┬─────┴────┬─────┴────┬─────┴──────────┘
      ▼          ▼          ▼          ▼
   mm->pgd → PUD表 → PMD表 → PTE表 → 物理页框 @ 4KiB
```

**为什么多级？** 单级 48-bit VA 需要 2^36 个 PTE（512 GiB 页表本身）；四级把稀疏地址空间的页表 **按需分配**——没映射的区域，上层表项为空，下层表根本不分配。代价：**每次 walk 最多 4 次内存访问**（TLB miss 时）。

**P4D 折叠（folded）：** 当 `CONFIG_PGTABLE_LEVELS=4` 时，`p4d_offset()` 直接返回 PGD 项本身——代码写五层、运行时少一层，这是内核 **同一套代码兼容 3/4/5 级页表**（x86-5 层 paging la57、ARM64 52-bit VA）的手段。**IA-5 层 paging（la57）开启后 P4D 真实展开。**

## 2. 类型系统：`pgd_t/p4d_t/pud_t/pmd_t/pte_t`

```c
typedef struct { pgdval_t pgd; } pgd_t;   /* 每个 arch 一份 */
typedef struct { pteval_t pte; } pte_t;
```

不是直接用 `unsigned long`，而是 **包一层 struct**——让类型检查器拦住「拿 PTE 当 PFN 用」的错误，并支持 **PAE 这类 PTE 位宽 ≠ 无符号字长** 的架构。配套取值宏：`pte_val(x)` / `__pte(v)`。

**PTE 各位的语义（架构无关抽象 + x86_64 对应）：**

| 抽象宏 | x86_64 硬件位 | 含义 | 谁会改它 |
|--------|---------------|------|----------|
| `_PAGE_PRESENT` | bit 0 | 物理页在内存 | 缺页 handler（换入后置位） |
| `_PAGE_RW` | bit 1 | 可写 | fork COW 时清零，写缺页恢复 |
| `_PAGE_USER` | bit 2 | 用户态可访问 | 内核映射清零 |
| `_PAGE_ACCESSED` | bit 5 | 硬件置位「访问过」 | 回收扫描后清零（ Aging） |
| `_PAGE_DIRTY` | bit 6 | 硬件置位「写脏了」 | 写回后清零 |
| `_PAGE_SPECIAL` | 软件位 | 非 `struct page` 支撑的映射（如 remap_pfn_range） | 建映射时决定 |
| `_PAGE_SOFT_DIRTY` | 软件位 | checkpoint/迁移用脏跟踪 | `/proc/pid/clear_refs` |
| swap entry | 全部软件复用 | PTE 非空但非 present → 编码 swap 槽位 | 换出路径 |

**关键：PTE 是「多义」的。** `pte_present()==false` 且项非空 → 这是 **swap entry / migration entry / file entry**，具体类型看编码。`pte_none()` 才是真空白。这是理解后面 Ch 11 swap 的地基。

## 3. 页表项 vs struct page flags 的分工

| | PTE 位 | `struct page` flags |
|---|---|---|
| 管什么 | **这一个虚拟映射**怎么看 | **这一个物理页**的全局状态 |
| 例子 | 本映射可写/被访问 | `PG_dirty`（文件页）、`PG_locked`、`PG_writeback` |
| 谁消费 | MMU 硬件 + 内核 walker | 回收/写回/rmap |

同一物理页映射到两个进程时只有一份 `struct page`，但两套 PTE——所以 dirty/young 信息 **可能同时存在于 PTE 位和 page flags**，回收时要在 `page_referenced()`（rmap 遍历）里把各级 PTE 的 accessed 位 **聚合** 到 page 上。

## 4. HFT / 嵌入式关联

| 现象 | 本节机制的兑现 |
|------|----------------|
| TLB miss 就是 4 次指针 chasing（每次可能 L1/L2 miss） | 页表 walk 成本 ≈ 4×cache miss；大页把 walk 结果缓存覆盖 2MiB/1GiB |
| PTE 里 `_PAGE_USER` 清零的内核映射 | 内核线程借用用户 mm 时（lazy TLB）安全的基础 |
| 观察进程真实映射 | `/proc/pid/pagemap` 每行就是一个 PFN+标志位（需 root） |
| 防侧信道 | `_PAGE_SPECIAL`/`pte_mknotpresent` 等；`mprotect` 清 `_PAGE_RW` 做 W^X |

## 5. 衔接

- 下节 [§2 遍历与使用页表](./section-2-遍历与使用页表.md)：内核代码怎么 **走** 这套表
- [§3 页表的分配与释放](./section-3-页表的分配与释放.md)：表页本身从哪来
- 现代演进：[06.5/ch04 页表与 TLB](../../../06.5-modern-mm/chapter-04-page-table-tlb/)
- 硬件视角：[15-体系结构 TLB](../../../15-computer-architecture/chapter-02-memory-hierarchy-design/)

---

<details>
<summary>代码自测（Q&A）</summary>

**Q1：为什么 P4D 在四级页表的 x86_64 上还存在？**
A：为了让一份内核代码同时服务 3/4/5 级配置。`pgtable.h` 里 `p4d_offset()` 在 `__PAGETABLE_P4D_FOLDED` 时退化为恒等返回（返回 PGD 项的地址当 P4D 用）。5 级（la57）开启时才真正分配 P4D 表。这就是"folded level"——**存在但无实体**。

**Q2：TLB miss 时 walk 页表的是硬件还是内核？**
A：x86_64/ARM64 都是 **硬件 page table walker**（MMU 自动走，OS 只负责填表+维护 TLB 一致性）。软件 walk 只发生在内核自己遍历（如 `follow_page`）或架构无硬件 walker（如早期 MIPS）。x86 还有 5-level paging 时 walk 从 CR3 → P4D → …，多一次访存。

**Q3：一个 PTE 8 字节、一页表页 4KiB 能放 512 项，为什么每级索引恰好 9 位？**
A：4KiB/8B = 2^9。这不是巧合——每级表恰好一页，让页表页可以 **用 buddy 分配器整页分配**，无需子页分配器。ARM64 64KiB 页时每级 13 位（8192 项）。

**Q4：进程 A、B 共享一个物理页（fork 后未 COW），`_PAGE_DIRTY` 在谁的 PTE 里？**
A：各自的。PTE 位是 **per-mapping** 的——A 写脏只置 A 的 PTE dirty 位。但 `struct page` 只有一份，文件页的脏状态以 page/address_space 为准，`vm_normal_page()` 拿到 page 后统一管理。这也是 rmap 存在的理由：从 page 反向找到所有 PTE。

**Q5：`CONFIG_PGTABLE_LEVELS` 从 4 变 5（la57），用户程序需要重编译吗？**
A：不需要——虚拟地址空间用户可见部分仍从 0 开始，只是可寻址上限变大（57-bit VA 128 PiB）。但 VA 布局（`TASK_SIZE`、mmap top）会变，内核选地址的默认值可能落在更高区域。

</details>

---
