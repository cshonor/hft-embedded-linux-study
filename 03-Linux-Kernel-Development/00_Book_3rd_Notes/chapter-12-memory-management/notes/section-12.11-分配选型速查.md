## 分配选型速查

| 需求 | API |
|------|-----|
| 若干连续物理页 | `alloc_pages` / `__get_free_pages` |
| 小对象、物理连续、快 | **`kmalloc`** |
| 中断/持锁、不能睡 | **`kmalloc(..., GFP_ATOMIC)`** |
| 大块、虚连续即可 | **`vmalloc`** |
| 固定类型高频对象 | **Slab / `kmem_cache_*`** |
| 每核私有计数/队列 | **per-CPU** |
| HIGHMEM 页访问 | **`kmap` / `kmap_atomic`** |

---
