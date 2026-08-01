# Ch 7 §1 描述虚拟内存区域 (`vm_struct`)

> **Understanding the Linux Virtual Memory Manager** · Mel Gorman · **跳过 ⚪**

### 1. 描述虚拟内存区域 (`vm_struct`)

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
