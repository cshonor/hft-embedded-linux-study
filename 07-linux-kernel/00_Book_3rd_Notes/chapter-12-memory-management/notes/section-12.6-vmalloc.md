## ⑥ vmalloc()

**虚拟连续、物理可不连续** — 用 **改页表** 把零散物理页拼成 **连续 VA 区间**，适合 **大块**、**非 DMA**、**非性能关键** 的内核分配。

#### 保证与代价

| 保证 | 不保证 |
|------|--------|
| **内核 VA 连续** | **物理连续** |
| 可分配 **很大** 区域（仅受 VA 空间限制） | **低延迟** |
| | **适合 DMA** |

| 代价 | 原因 |
|------|------|
| **比 kmalloc 慢** | 需 **建立页表项**、可能 **缺页** 填 PTE |
| **TLB 压力** | 每页一项 — **miss 多** |
| **更碎片化物理** | 从 buddy 逐页凑 |

```
kmalloc 路径:
  [ 物理连续 4KB×N ] ──direct map──► 连续 VA  （快）

vmalloc 路径:
  物理页 A   物理页 C   物理页 F  （分散）
     │          │          │
     └──── 页表拼接 ────► [ VA: v .. v+size )  连续
```

#### API（概念）

| API | 说明 |
|-----|------|
| **`vmalloc(size)`** | 分配 **size** 字节虚连续区 |
| **`vzalloc(size)`** | + 清零 |
| **`vmalloc_user(size)`** | 可 **mmap 到用户态** 的特殊区 |
| **`vfree(addr)`** | 释放 — **必须配对** |

#### 典型用途

| 场景 | 为何 vmalloc |
|------|--------------|
| **内核模块加载** | 代码/数据 **大块**、加载期一次 |
| **大表 / debug** | `/proc` 大缓冲、驱动 **big config table** |
| **filesystem 元数据** | 非热路径大结构 |

#### 与 `ioremap`

| API | 对象 |
|-----|------|
| **`vmalloc`** | **物理 RAM** 页 |
| **`ioremap`** | **MMIO 设备寄存器** — 非 RAM |

#### 何时 **不要** vmalloc

| 场景 | 应用 |
|------|------|
| **网络包处理热路径** | **`kmalloc` / 预分配池** |
| **DMA 缓冲** | **`dma_alloc_coherent`** / **`alloc_pages`** |
| **中断上下文频繁 alloc** | **Slab cache 预建** |

**HFT：** 用户态 **`mmap` 大块匿名区** 与 **`vmalloc` 思想类似 — **虚连续**。但 **策略 ring** 还要 **`mlock` + hugepage** 保证 **物理稳定 + TLB 友好** — 内核 **`vmalloc` 无 hugepage 语义**，热路径 **禁用**。

→ [06 Gorman Ch7 非连续分配](../../../../09-linux-mm/chapter-07-noncontiguous-memory-allocation/) · [Ch 15 mmap 用户视角](../../chapter-15-process-address-space/) · [01 CSAPP Ch9](../../../../02-computer-systems/chapter-09-virtual-memory/)


> ↔ [ULK Ch8 §4 非连续内存与vmalloc](../../../../08-linux-kernel-deep/chapter-08-memory-management/notes/section-4-非连续内存与vmalloc.md)
---
