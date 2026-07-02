# Ch 8 §3 对象分配与释放

> **Understanding the Linux Virtual Memory Manager** · Mel Gorman · **精读 🔴**

### 3. 对象分配与释放

### 分配：`kmem_cache_alloc()`

```
kmem_cache_alloc(cache)
    │
    ├─ per-CPU 本地池有？ ──► 直接取（§5）
    │
    └─ 从 cache->slabs_partial 取 object
           │
           无 partial ──► kmem_cache_grow()
                              ├─ Buddy alloc_pages 建新 slab
                              ├─ 划分 objects
                              └─ 调用 constructor 初始化（仅新 object）
```

### 释放：`kmem_cache_free()`

```
kmem_cache_free(cache, obj)
    │
    ├─ 优先还 per-CPU 本地池
    │
    └─ 标记 object 空闲（bufctl / freelist）
           │
           slab 变全空？ ──► 可能移入 slabs_free（稍后 shrink 或 destroy）
```

**HFT：** **grow** = 向 OS 要新页（慢路径）；**partial 命中** = 池内 pop（快路径）— 与 **mempool cache 批量 refill** 同构。

---
