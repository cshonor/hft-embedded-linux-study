# Ch 7 非连续内存分配 · Noncontiguous Memory Allocation

> **Understanding the Linux Virtual Memory Manager** · Mel Gorman · **跳过**（HFT 热路径 **优先物理连续大页**；`vmalloc` 适合 **大块内核模块/驱动** — 理解 **与 `kmalloc`/Buddy 的分工** 即可）

## 为什么需要 vmalloc

[Ch 6 Buddy](./chapter-06-物理页分配.md) 分配 **物理连续** 的 **2^n 页** — 快、缓存友好。但 **外部碎片** 积累后，可能 **凑不出大块连续物理内存**，尽管 **总空闲页足够**。

**`vmalloc()` 的取舍：**

| | **Buddy / `__get_free_pages`** | **`vmalloc()`** |
|---|-------------------------------|-----------------|
| **物理内存** | **连续** | **不连续**（每页独立 alloc） |
| **虚拟地址** | 内核线性映射或固定映射 | **虚拟连续** 一段 VA |
| **TLB / 缓存** | 较好 | **每页独立映射** — TLB 压力更大 |
| **典型用途** | 热路径、DMA、大页 | **大但不必物理相邻** 的内核缓冲区、模块加载 |

→ Ch 1 阅读路线 **第 2 步**：[`mm/vmalloc.c`](https://elixir.bootlin.com/linux/latest/source/mm/vmalloc.c)

---

## 本章在 VM 子系统中的位置

```
Ch 6 Buddy：要连续物理页，失败时可能 EMEM（碎片）
        ↓
Ch 7 vmalloc：虚拟连续、物理可散 — 用页表「拼」出来
        ↓
Ch 8 slab：小块对象 — 通常 Buddy 拿整页再切
Ch 4 fault：vmalloc 区访问时同步页表项
```

**HFT：** 用户态 **订单簿 / ring buffer** 应 **`mmap` + hugepage / 预 fault 连续页** — **不要** 模仿 vmalloc（内核 API）。内核模块若大块非 DMA 缓冲，才可能走 vmalloc。

---

## 1. 描述虚拟内存区域 (`vm_struct`)

### 内核 vmalloc 区

内核在 **虚拟地址空间** 保留专用区间：

```
    …
    VMALLOC_START  ═══════════════════════════  vmalloc 区
                   │  vm_struct #1            │
                   │  (guard page)            │
                   │  vm_struct #2            │
                   …
    VMALLOC_END    ═══════════════════════════
    …
```

| 概念 | 说明 |
|------|------|
| **`VMALLOC_START` ~ `VMALLOC_END`** | **非连续分配** 专用的 **内核虚拟地址窗口**（与 **线性映射 `PAGE_OFFSET` 区**、**用户空间** 分离） |
| **`vm_struct`** | 描述 **一段 vmalloc 区域** — **不是** 用户态的 **`vm_area_struct` (VMA)** |
| **链表 `next`** | 各 **`vm_struct`** 串成链表 |
| **Guard page** | 相邻区域之间 **至少隔一个未映射页** — 防 **越界写** 悄悄破坏邻区 |

**`vm_struct` 管的是「内核虚拟窗口里的段」**；用户进程 **`mmap`** 用的是 **`vm_area_struct`**（Ch 4）— **两套描述符，别混**。

---

## 2. 分配非连续区域 (Allocating)

### API

| 函数 | 说明 |
|------|------|
| **`vmalloc(size)`** | 通用 — 从 **NORMAL 等 zone** 取散页 |
| **`vmalloc_dma()`** | 物理页需满足 **DMA 可达** 约束 |
| **`vmalloc_32()`** | 物理页落在 **32 位可寻址** 范围（旧架构 / 设备） |

底层按 **GFP / zone 策略** 调用 **`alloc_page()`** 等（Ch 6）— **每页独立**，不要求彼此物理相邻。

### 两步分配

```
vmalloc(size)
    │
    ├─ (1) get_vm_area()
    │       在 VMALLOC_START~END 找足够大的 **空闲虚拟区间**
    │       新建 vm_struct，链入全局链表
    │
    └─ (2) 建立页表映射
            分配 PGD/PMD/PTE（Ch 3）
            对每个虚拟页：alloc_page() → 填入 PTE
            （2.6：见 §4 — 先攒页数组再 map_vm_area）
```

### 页表同步：`init_mm` vs 当前进程

**关键：** `vmalloc()` 首先更新的是 **内核参考页表 `init_mm->pgd`** — **不是** 每个进程的 PGD。

| 何时 | 发生什么 |
|------|----------|
| **vmalloc 返回后** | 参考页表已有映射；**当前进程 PGD 可能还没有** |
| **首次访问该 VA** | **缺页异常** → 发现地址落在 **vmalloc 区** → 把 **`init_mm` 中对应 PTE** **复制/同步** 到 **当前进程页表** |

**直觉：** vmalloc 区是 **内核共享资源**；**懒同步** 到各进程（实为内核线程/用户态触达时的 **内核页表视图**）— 与 **用户 mmap demand fault** 类似，但 **走内核专用 fault 分支**。

→ [Ch 4 缺页](./chapter-04-进程地址空间.md#4-异常处理与缺页异常-page-faulting) · [Ch 3 PTE](./chapter-03-页表管理.md)

---

## 3. 释放非连续区域 (Freeing)

**`vfree(addr)`** 大致步骤：

```
vfree
    → 扫描 vm_struct 链表，定位包含 addr 的 vm_struct
    → vmfree_area_pages()
           反向 walk 页表
           清除 PTE 映射
           free 每个物理页（回 Buddy）
           释放页表页（若不再使用）
    → 从链表移除 vm_struct，释放 guard 与描述符
```

**与 vmalloc 对称** — unmap + **`free_pages`** 逐页归还。

---

## 4. 2.6 内核的新变化

2.4 vs 2.6 **架构相似**，**分配物理页的时机** 有变：

| 2.4 | 2.6 |
|-----|-----|
| **walk 页表到 PTE 时** 才 **逐个** `alloc_page` | **`vmalloc()` 先分配齐** 全部物理页，放入 **数组** |
| 边建表边要页 | 一次 **`map_vm_area()`** 统一插入内核页表 |

**效果：** 映射路径 **更集中**；失败时 **更容易整体回滚**（原书动机）。现代 `vmalloc.c` 仍保留 **「先 reserve VA + 再 map 物理页」** 两阶段思想。

---

## vmalloc vs Buddy vs slab 一图

```
需要 N 字节内核内存
        │
        ├─ 小对象、高频？ ──► slab / kmalloc (Ch 8)
        │
        ├─ 物理必须连续、快？ ──► Buddy __get_free_pages (Ch 6)
        │       └─ 失败（碎片）？
        │
        └─ 虚拟连续即可、较大块？ ──► vmalloc (Ch 7)
                VA 连续，物理散页 + 页表拼接
                首次 touch 可能 fault 同步 PTE
```

---

## HFT / 阅读建议

| 场景 | 建议 |
|------|------|
| **低延迟用户态堆** | **物理连续 + 大页 + mlock** — 不走 vmalloc |
| **内核驱动大块缓冲** | 能 **连续** 则 Buddy；否则 **vmalloc** 或 **CMA**（现代扩展） |
| **理解 fault 延迟** | vmalloc 区 **首访** = 页表同步 + 可能 TLB miss 链 |
| **读源码** | [`mm/vmalloc.c`](https://elixir.bootlin.com/linux/latest/source/mm/vmalloc.c) + `mm/vmalloc.h` |

---

## 相关章节

- 上一章：[chapter-06-物理页分配.md](./chapter-06-物理页分配.md)
- 下一章：[chapter-08-Slab分配器.md](./chapter-08-Slab分配器.md)
- 附录 G：[appendix-G-非连续内存分配.md](./appendix-G-非连续内存分配.md)
- Ch 1 路线第 2 步 · [chapter-01-简介.md](./chapter-01-简介.md#4-阅读代码的策略-reading-the-code)

---
