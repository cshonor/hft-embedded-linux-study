# Ch 3 页表管理 · Page Table Management

> **Understanding the Linux Virtual Memory Manager** · Mel Gorman · **精读 🔴**

---

## 本章概述

> **Understanding the Linux Virtual Memory Manager** · Mel Gorman · **精读**

本章讲 Linux 如何以 **与硬件体系结构无关** 的方式 **管理、组织页表** — 把线性（虚拟）地址 **逐级查表** 落到 **物理页框**（Ch 2 的 `struct page`）。

> **时代说明：** 原书以 **2.4 / 2.6**、**三级页表 (PGD → PMD → PTE)** 叙述（32 位 x86 / PAE 语境）。**x86_64 现代内核** 通常为 **四级**（PGD → PUD → PMD → PTE，见 `asm/pgtable*.h`）；**架构无关层** 仍用 `pgd_t` / `pmd_t` / `pte_t` 抽象，层数由 `CONFIG_PGTABLE_LEVELS` 决定。读源码以当前树为准。

> **HFT 延伸：** 透明大页 THP → [./note-透明大页THP.md](./note-透明大页THP.md)

---

## 本章在 VM 子系统中的位置

```
Ch 2：物理页框 (struct page) 在哪、属哪个 Zone
        ↓
Ch 3：页表 — 虚拟地址 → PTE → 物理 PFN
        ↓
Ch 4：每个进程的 mm_struct / VMA 如何挂在这套页表上
        ↓
Ch 10 / swap：改 PTE、rmap 解映射、换出
```

**HFT 为什么要读：** 每次 **TLB miss** 可能触发 **页表 walk**（多级指针 chasing，cache miss 链）；**大页** 减少 TLB 覆盖范围；**mlock** 保证 **PTE present**；**绑核 + 进程迁移** 涉及 **TLB flush**（`flush_tlb_*`）。

→ 交叉：[01-CSAPP Ch9 §9.6–9.7](../01-CSAPP-3rd/chapter-09-virtual-memory/) · [03-Linux-Kernel-Development Ch15 §15.7 页表](../03-Linux-Kernel-Development/00_Book_3rd_Notes/chapter-15-process-address-space/notes/section-15.7-页表.md)

---

## 小节笔记

| 节 | 笔记 |
|----|------|
| 1. 页目录与页表项 | [notes/section-1-页目录与页表项.md](./notes/section-1-页目录与页表项.md) |
| 2. 遍历与使用页表 | [notes/section-2-遍历与使用页表.md](./notes/section-2-遍历与使用页表.md) |
| 3. 页表的分配与释放 | [notes/section-3-页表的分配与释放.md](./notes/section-3-页表的分配与释放.md) |
| 4. 内核页表初始化 | [notes/section-4-内核页表初始化.md](./notes/section-4-内核页表初始化.md) |
| 5. 地址与 struct page 的映射 | [notes/section-5-地址与-struct-page-的映射.md](./notes/section-5-地址与-struct-page-的映射.md) |
| 6. TLB 与 L1 Cache 管理 | [notes/section-6-TLB-与-L1-Cache-管理.md](./notes/section-6-TLB-与-L1-Cache-管理.md) |
| 7. 2.6 内核的新变化 | [notes/section-7-2.6-内核的新变化.md](./notes/section-7-2.6-内核的新变化.md) |
| HFT 延伸 · THP | [notes/note-透明大页THP.md](./notes/note-透明大页THP.md) |

---

## 相关章节

- 上一章：[../chapter-02-describing-physical-memory/](../chapter-02-describing-physical-memory/)
- 下一章：[../chapter-04-process-address-space/](../chapter-04-process-address-space/)
- 附录 C：[../../appendix-C-页表管理.md](../../appendix-C-页表管理.md)
- 全书目录：[OUTLINE.md](../../OUTLINE.md)
