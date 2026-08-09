# Ch 8 §2 核心数据结构：Cache 与 Slab

> **Understanding the Linux Virtual Memory Manager** · Mel Gorman · **精读 🔴**

### 2. 核心数据结构：Cache 与 Slab

### 缓存 `kmem_cache_t`

| 概念 | 说明 |
|------|------|
| **Cache chain** | 各 **`kmem_cache`** 双向链表串起 — 每种 **内核对象类型** 一个 cache（如 `task_struct`、`inode`、`mm_struct`） |
| **三条 Slab 链** | **`slabs_full`** · **`slabs_partial`** · **`slabs_free`** |

```
kmem_cache (e.g. "mm_struct")
    slabs_full     ──► 无空闲 object
    slabs_partial  ──► 有闲有占 ← 分配首选
    slabs_free     ──► 全空，可还给 Buddy
```

### Slab `slab_t`

| 概念 | 说明 |
|------|------|
| **组成** | **一个或多个连续物理页**（来自 Buddy），划分为 **N 个 object slot** |
| **描述符位置** | **on-slab**（小对象，描述符在页内）或 **off-slab**（描述符单独分配） |

### 空闲对象跟踪：`kmem_bufctl_t`

不用 **逐对象链表**，而用 **整型数组** 作 **LIFO 索引栈** — **O(1)** 取空闲 object、归还 object。

（SLUB 用 **freelist 指针嵌入 object 头部** 等变体 — **思想同为 LIFO 热缓存**。）

---
