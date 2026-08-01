# Ch 3 §7 2.6 内核的新变化 (What's New in 2.6)

> **Understanding the Linux Virtual Memory Manager** · Mel Gorman · **精读 🔴**

### 7. 2.6 内核的新变化 (What's New in 2.6)

### 无 MMU 架构 (`mm/nommu.c`)

为 **无 MMU** 微控制器提供 **nommu** 路径 — 无硬件页表，**uClinux** 等场景；与 **HFT 服务器** 关系不大，知道 **Linux MM 有 fork** 即可。

### 反向映射 (Reverse Mapping · rmap) — **重点**

| 2.4 痛点 | 2.6 rmap |
|----------|----------|
| **共享页** 换出时需 **扫描所有进程页表** 找指向该页的 PTE — **O(进程×页表)** | 每个 **`struct page`** 挂 **映射它的 PTE 链表 (anon_vma / address_space)** |
| 解除映射慢、难原子 | **直接找到并解除所有 PTE**，换出 (**pageout**) **快得多** |

**至今仍是核心：** 现代 **`mm/rmap.c`** — **RMAP** 与 **LRU 回收、swap、munmap、madvise** 交织。

```
共享物理页 P
    struct page ──► rmap 链表 ──► [mm₁,vma₁,pte₁] [mm₂,vma₂,pte₂] …
    换出时：沿链表 clear PTE，而非扫全局
```

→ [Ch 10 页框回收](../../chapter-10-page-frame-reclamation/) · [Ch 12 共享内存](../../chapter-12-shared-memory-virtual-filesystem/)（文件页 rmap 经 **address_space**）。

### 高端内存中的 PTE (PTEs in High Memory)

大内存机器上 **PTE 表本身** 也可能占满 **ZONE_NORMAL** 低端：

- 2.6 允许 **PTE 页分配在 ZONE_HIGHMEM**
- 访问时用 **`pte_offset_map()`** **临时映射** 到低端可触达虚拟地址，用完 **`pte_offset_unmap()`**

**64 位** 上问题缓解，但 **「页表本身也占物理内存、也占低端窗口」** 的思想仍在 **内存紧张 / 嵌入式** 场景出现。

---

## 三级页表 + 地址转换一图（原书模型）

```
                    mm_struct
                        │
                        ▼
              ┌─────────────────┐
    VA ──────►│ PGD ──► PMD ──► PTE ──► PFN ──► struct page
              └─────────────────┘
                        │
              PTE bits: PRESENT RW USER DIRTY ACCESSED …
                        │
              TLB 缓存 VA→PA；未命中则 walk 上述层级
```

---

## HFT 精读 checklist

| 主题 | 行动 |
|------|------|
| **TLB / 页表 walk** | 压工作集、**大页**、减少 pointer chasing 数据结构 |
| **PTE present + mlock** | 避免 **缺页** 与 **swap** 路径 |
| **flush_tlb_*** | 理解 **绑核**、**减少跨核页表改动** |
| **rmap** | 理解 **共享内存 / mmap 文件** 换出成本 — 多映射页更「重」 |
| **THP** | 在 **减少 TLB miss** 与 **合并延迟抖动** 之间取舍 → [./note-透明大页THP.md](./note-透明大页THP.md) |

---
