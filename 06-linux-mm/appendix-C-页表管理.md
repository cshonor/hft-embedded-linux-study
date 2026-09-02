# 附录 C 页表管理 · Page Table Management

> **Code Commentary** · Mel Gorman · **选读** · 源码核验：Linux v6.6

概念总览 → [./chapter-03-page-table-management/](./chapter-03-page-table-management/)

---

## 本节走读什么

正文 Ch3 讲了「页表层级、PTE 位、TLB」。本附录走读**页表的代码组织**：缺页处理的入口 `handle_mm_fault`（`mm/memory.c`）、页表复制/清理（`copy_pte_range` / `zap_pte_range`）、反向映射（`rmap` 的 `anon_vma`）。

---

## 1. 缺页处理的三分叉（mm/memory.c）

`do_user_addr_fault`（arch/x86/mm/fault.c:1239，架构侧入口）→ `handle_mm_fault` → `__handle_mm_fault` → 按「缺的是什么页」分叉：

```
__handle_mm_fault(mm, vma, address, flags)          // memory.c:3670 附近
        │
        ├─ 匿名页（首次访问，无 backing）→ do_anonymous_page()   // :107 声明
        │       └─ 分配零页 / 按需分配，建 PTE
        ├─ 文件映射缺页               → do_fault()              // :106 声明
        │       └─ 从 page cache 读入 / 建立 file mapping
        └─ 换出页缺页                 → do_swap_page()          // :3722
                └─ 从 swap 换入（Ch11）
```

三个函数都返回 `vm_fault_t`（一组 `VM_FAULT_*` 标志），把「怎么处理这次缺页」的结果反馈给上层。

**写时复制**：`do_wp_page()`（:3338）处理「进程要写一个只读页」——COW 语义下分配新页、拷贝内容、改 PTE 为可写。这是 fork 之后父子进程共享页、写时才真正分离的核心（Ch4 COW）。

## 2. 页表复制与清理

| 函数 | 行号 | 作用 |
|------|------|------|
| `copy_pte_range` | memory.c:1001 | fork 时复制父进程的 PTE（COW 下共享页、只读化） |
| `zap_pte_range` | memory.c:1394 | munmap/exit 时清理 PTE、归还页框（配合 `mmu_gather` 批量 TLB flush） |
| `apply_to_page_range` | memory.c:2755 | 遍历一段地址范围的页表，对每个 PTE 执行回调（vmalloc 建立映射也用它） |

**走读要点**：`copy_pte_range` 和 `zap_pte_range` 是**一对镜像**——一个建（fork）、一个拆（exit/unmap）。两者都要**逐级遍历 PGD→P4D→PUD→PMD→PTE**，代码里是层层 `do ... while` 的展开（memory.c:1001-1167 可见 PTE 级的循环）。

## 3. 反向映射 `anon_vma`（include/linux/rmap.h:31）

正向映射是「页表 → 物理页」（通过 PTE），反向映射（rmap）是「物理页 → 所有引用它的 VMA」——回收时（Ch10 `try_to_unmap`）要把所有映射这条页的 PTE 都失效。

```c
struct anon_vma {
    struct anon_vma *root;       // 该 anon_vma 树的根
    struct anon_vma *parent;     // 父 anon_vma
    ...                          // 一个 anon_vma 挂一串「关联 VMA」
};
```

**为什么用 `anon_vma` 而不是直接指向 VMA**：因为一个匿名页可能被 fork 后的**多个进程的多个 VMA** 共享，直接指向单个 VMA 无法枚举全部。`anon_vma` 把「引用同一组匿名页的 VMA」串成链表，`try_to_unmap_one` 遍历这条链逐个失效 PTE。

## 4. 页表通用层 `pgtable-generic.c`

`mm/pgtable-generic.c` 提供**架构无关**的页表操作（各架构 `pgtable.h` 只提供底层原语）：

| 函数 | 作用 |
|------|------|
| `pmd_alloc` / `pte_alloc_map` | 按需分配下级页表 |
| `ptep_clear_flush` | 清 PTE + 刷 TLB |
| `pagetable_alloc` / `pagetable_free` | 页表页的分配/释放 |

**走读要点**：这是「架构相关 ↔ 无关」的分界——x86_64 的 5 级页表折叠（P4D 在 4 级时折叠成空操作，见 `asmgeneric_pgtable-nop4d.h` 缓存）就在这一层体现。

---

## 与正文对应

| 附录内容 | 正文落点 |
|----------|----------|
| `__handle_mm_fault` 三分叉 | Ch3（页表层级）+ Ch4（缺页处理流程） |
| `do_wp_page` COW | Ch4（写时复制） |
| `anon_vma` 反向映射 | Ch3（rmap）+ Ch10（try_to_unmap） |
| P4D 折叠 | Ch3（4/5 级页表，`pgtable-64_types.h`） |

---

## HFT / 嵌入式关联

**COW 是延迟陷阱**：`do_wp_page` 的「写只读页 → 分配新页 + 拷贝」会在热路径上引入**不可预期的分配延迟**。HFT 里对共享内存（shmem，附录 L）的理解要特别注意 COW 时机——一旦写共享页触发 COW，可能连带一次 page_alloc + memcpy。

---

## 相关章节

- 上一章：[appendix-B-描述物理内存.md](./appendix-B-描述物理内存.md)
- 下一章：[appendix-D-进程地址空间.md](./appendix-D-进程地址空间.md)

---

<details>
<summary>自测 5 问（点开看答案）</summary>

**Q1：缺页处理在 `__handle_mm_fault` 里按什么三分叉？**

匿名页（`do_anonymous_page`）、文件映射缺页（`do_fault`）、换出页缺页（`do_swap_page`），三个函数都返回 `vm_fault_t`。

**Q2：`do_wp_page` 处理什么？为什么它是延迟陷阱？**

处理「写只读页」的写时复制（COW）——分配新页 + 拷贝内容 + 改 PTE 可写（memory.c:3338）。因为它在写那一刻才触发 page_alloc + memcpy，延迟不可预期。

**Q3：`copy_pte_range` 和 `zap_pte_range` 是什么关系？**

一对镜像：前者 fork 时复制父进程 PTE（COW 下共享页只读化），后者 munmap/exit 时清理 PTE、归还页框（配合 mmu_gather 批量 TLB flush）。

**Q4：rmap 为什么用 `anon_vma` 而不是直接指向 VMA？**

因为一个匿名页可能被 fork 后多个进程的多个 VMA 共享，直接指向单个 VMA 无法枚举全部；`anon_vma` 把「引用同一组匿名页的 VMA」串成链表供 `try_to_unmap_one` 遍历。

**Q5：`pgtable-generic.c` 和架构 `pgtable.h` 的分工？**

前者提供架构无关的页表操作（pmd_alloc/ptep_clear_flush/页表页分配），后者只提供底层原语；x86_64 的 4 级页表下 P4D 折叠成空操作就在这一层体现。

</details>
