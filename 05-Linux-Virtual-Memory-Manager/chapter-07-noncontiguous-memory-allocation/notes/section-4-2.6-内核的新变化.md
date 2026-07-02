# Ch 7 §4 2.6 内核的新变化

> **Understanding the Linux Virtual Memory Manager** · Mel Gorman · **跳过 ⚪**

### 4. 2.6 内核的新变化

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
