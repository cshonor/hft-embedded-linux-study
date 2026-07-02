## ⑥ Slab 层 · Slab Layer

频繁分配/释放 **固定大小对象**（`task_struct`、inode…）→ **Slab 对象缓存**。

| 概念 | 说明 |
|------|------|
| **Cache** | 一种对象类型一条缓存 |
| **Slab** | 一条缓存里若干 **页块**，状态 **满 / 半满 / 空** |
| 分配策略 | 优先 **半满 Slab** 的空槽 — **快 + 减碎片** |

| API | 作用 |
|-----|------|
| **`kmem_cache_create()`** | 建自定义缓存 |
| **`kmem_cache_alloc()`** | 从缓存取对象 |
| **`kmem_cache_free()`** | 归还 |

→ **Ch 3** `task_struct` 由 Slab 分配

---
