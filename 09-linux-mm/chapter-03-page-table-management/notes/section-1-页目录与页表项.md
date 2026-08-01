# Ch 3 §1 页目录与页表项 (PGD / PMD / PTE)

> **Understanding the Linux Virtual Memory Manager** · Mel Gorman · **精读 🔴**

### 1. 页目录与页表项 (PGD / PMD / PTE)

### 三级结构（原书 · 架构无关模型）

| 层级 | 名称 | 作用 |
|------|------|------|
| **PGD** | Page Global Directory · 页全局目录 | 每进程 **`mm_struct`** 有指向 **自身 PGD** 的指针 |
| **PMD** | Page Middle Directory · 页中间目录 | PGD 项指向 PMD 表 |
| **PTE** | Page Table Entry · 页表项 | PMD 项指向 PTE 表；**PTE 指向物理页框**（用户数据所在） |

```
线性地址  ──►  PGD[index0]  ──►  PMD[index1]  ──►  PTE[index2]  ──►  物理页框
                mm_struct->pgd
```

**x86_64 注：** 在 PGD 与 PMD 之间多一层 **PUD**（Page Upper Directory）；**Linux 通用宏**（`pgd_offset`、`p4d_offset`、`pud_offset`、`pmd_offset`、`pte_offset`）在各级 arch 上展开不同深度。

### 类型与地址拆分宏

| 类型 | 用途 |
|------|------|
| **`pgd_t` / `pmd_t` / `pte_t`** | 类型安全的页表项容器；支持 **PAE** 等扩展物理地址位宽 |
| **`SHIFT` / `SIZE` / `MASK` 宏** | 把 **线性地址** 拆成各级 **索引偏移** |

### PTE 保护位与状态位（示例）

| 位 / 宏 | 含义 |
|---------|------|
| **`PAGE_PRESENT`** | 页在 **内存中**（非 swap / 非 hole） |
| **`PAGE_RW`** | **可写** |
| **`PAGE_USER`** | **用户态** 可访问（无则仅内核） |
| **`PAGE_DIRTY`** | 已被写入 — **脏页**，需写回 |
| **`PAGE_ACCESSED`** | 被访问过 — LRU / 回收参考（常对应 **young** 位） |

→ 与 Ch 2 **`PG_dirty` / `PG_active`** 等 **struct page flags** 协同；换出、写回路径会 **同时** 动 PTE 与 page flags。

---
