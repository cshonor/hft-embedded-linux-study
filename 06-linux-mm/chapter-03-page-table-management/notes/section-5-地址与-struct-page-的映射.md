# Ch 3 §5 地址与 struct page 的映射

> **Understanding the Linux Virtual Memory Manager** · Mel Gorman · **精读 🔴**

### 5. 地址与 struct page 的映射

内核需在 **物理地址 ↔ 虚拟地址 ↔ struct page** 之间 **快速转换**。

### 物理地址 ↔ 内核虚拟地址（ZONE_NORMAL）

在 **内核直接映射区**（x86 32 位上约 **3GiB 起 `PAGE_OFFSET`**）：

| 方向 | 思路 |
|------|------|
| 物理 → 虚拟 | **`phys + PAGE_OFFSET`**（宏 **`__va()`** / **`phys_to_virt()`**） |
| 虚拟 → 物理 | 减 **`PAGE_OFFSET`**（**`__pa()`** / **`virt_to_phys()`**） |

**仅适用于「线性映射窗口」内的物理页** — HIGHMEM 需 `kmap`（Ch 2 §4）。

### 物理地址 → struct page

1. 物理地址 **右移** 得到 **页框号 PFN**
2. 全局 **`mem_map[]`**（现代：**sparse vmemmap** / **pfn_to_page**）— **PFN 作索引** 取 **`struct page`**
3. 宏 **`virt_to_page()`** / **`pfn_to_page()`** 封装上述逻辑

```
PFN  ──index──►  mem_map[PFN]  ==  struct page  （Ch 2）
                      ▲
PTE ──frame number──┘
```

→ **页表管「虚拟→物理」；mem_map/ vmalloc 管「物理→struct page 元数据」**。

---
