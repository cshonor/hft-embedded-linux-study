# Ch 7 §2 分配非连续区域 (Allocating)

> **Understanding the Linux Virtual Memory Manager** · Mel Gorman · **跳过 ⚪**

### 2. 分配非连续区域 (Allocating)

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

→ [Ch 4 缺页](../../chapter-04-process-address-space/notes/section-4-异常处理与缺页异常.md#4-异常处理与缺页异常-page-faulting) · [Ch 3 PTE](../../chapter-03-page-table-management/)

---
