## ⑤ 操作内存区域

| 函数 | 作用 |
|------|------|
| **`find_vma(mm, addr)`** | 在 `mm_rb` 中找 **第一个 `vm_end > addr`** 的 VMA（含覆盖 addr 的区） |
| **`mmap_cache`** | 缓存上次查找 — **加速局部性重复访问** |
| **`find_vma_prev()`** | 找前一个 VMA |
| **`find_vma_intersection()`** | 找与给定区间 **相交** 的 VMA |

---
