# Ch 14 §4 VM 子系统交互全局概览（对应原书图 14.1）

> **Understanding the Linux Virtual Memory Manager** · Mel Gorman · **跳过 ⚪**

### 4. VM 子系统交互全局概览（对应原书图 14.1）

宏观上，**一次用户态访问内存** 可能穿过 **整条链**；**一次 `malloc`/fault** 则从下往上 **要页**：

```
┌─────────────────────────────────────────────────────────────────┐
│  用户进程：mmap / brk / touch VA / MAP_SHARED IPC               │
└────────────────────────────┬────────────────────────────────────┘
                             ▼
┌─────────────────────────────────────────────────────────────────┐
│  Ch 4  mm_struct · VMA · page fault · COW · copy_*_user         │
└────────────────────────────┬────────────────────────────────────┘
                             ▼
┌─────────────────────────────────────────────────────────────────┐
│  Ch 3  PGD/PMD/PTE · TLB flush · rmap                           │
└────────────────────────────┬────────────────────────────────────┘
                             ▼
┌─────────────────────────────────────────────────────────────────┐
│  Ch 2  struct page · Zone · Node · LRU flags · pageset           │
└────────────────────────────┬────────────────────────────────────┘
                             ▼
        ┌────────────────────┼────────────────────┐
        ▼                    ▼                    ▼
   Ch 6 Buddy          Ch 8 Slab            Ch 7 vmalloc
   alloc_pages         kmalloc              虚拟连续/物理散
        ▲                    ▲
        │                    │
   Ch 5 bootmem/memblock 退役移交
        │
   内存不足时 ↑
        │
   Ch 10 shrink · kswapd · page cache
        ├─► Ch 11 swap（匿名页）
        └─► Ch 12 shmem/tmpfs（共享「文件」页）
        │
   仍不足 ► Ch 13 OOM killer
```

### 按章快速索引（理论部分）

| 章 | 主题 | 笔记 |
|:--:|------|------|
| 1 | 读源码 / 补丁 / `mm/` 路线 | [chapter-01](../../chapter-01-introduction/) |
| 2 | Node · Zone · `struct page` | [chapter-02](../../chapter-02-describing-physical-memory/) |
| 3 | 页表 · TLB · rmap | [chapter-03](../../chapter-03-page-table-management/) |
| 4 | VMA · fault · mlock | [chapter-04](../../chapter-04-process-address-space/) |
| 5 | bootmem → Buddy | [chapter-05](../../chapter-05-boot-memory-allocator/) |
| 6 | 伙伴系统 · GFP | [chapter-06](../../chapter-06-physical-page-allocation/) |
| 7 | vmalloc | [chapter-07](../../chapter-07-noncontiguous-memory-allocation/) |
| 8 | Slab · kmalloc · mempool | [chapter-08](../../chapter-08-slab-allocator/) |
| 9 | HIGHMEM · kmap · bounce | [chapter-09](../../chapter-09-high-memory-management/) |
| 10 | LRU · kswapd · reclaim | [chapter-10](../../chapter-10-page-frame-reclamation/) |
| 11 | swap · swp_entry | [chapter-11](../../chapter-11-swap-management/) |
| 12 | tmpfs/shmem · SysV shm | [chapter-12](../../chapter-12-shared-memory-virtual-filesystem/) |
| 13 | OOM | [chapter-13](../../chapter-13-out-of-memory-management/) |

### HFT 精读捷径（相对作者 Ch1 源码路线）

| 路径 | 章节 |
|------|------|
| **延迟 / 布局** | Ch 2 → 3 (+ [THP](../../chapter-03-page-table-management/notes/note-透明大页THP.md)) → 8 → 4 → 10 |
| **读 `mm/` 源码** | Ch 1：`oom_kill.c` → `vmalloc.c` → `page_alloc.c` → `mmap.c` |

---
