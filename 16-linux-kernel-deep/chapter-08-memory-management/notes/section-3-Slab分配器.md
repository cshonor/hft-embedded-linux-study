## 3. 内存区管理 · Slab 分配器

> 伙伴系统适合 **页级大块**；几十字节的小对象需要 **Slab**

---

### 一、内碎片 vs 外碎片

| 类型 | 含义 |
|------|------|
| **外碎片** | 有空闲页，但不 **连续** — 伙伴系统解决 |
| **内碎片** | 为 32B 请求分配整页 — **浪费页内空间** — Slab 解决 |

---

### 二、Slab 三层结构

```
高速缓存 (Cache)  — 同一类型内核对象（如 dentry、inode）
    ↓
Slab            — 一个或多个连续页框，切成多个对象
    ↓
Object          — 实际分配单元（已用 / 空闲）
```

- 减少对 **伙伴系统** 的调用  
- 提高 **cache 命中率**、分配速度  

→ 07 Gorman / 05 LKD 有 modern **SLUB/SLOB** 演进；ULK 讲 **Slab 概念**。

---

### 三、Slab 着色 (Slab Coloring)

- 不同 Slab 中 **相同偏移** 的对象 → 易映射到同一 **CPU cache line** → 冲突  
- **着色：** 利用 Slab 末尾空闲字节，让各 Slab 对象 **起始偏移不同** → 分散 cache line  

---

### 四、通用对象与 `kmalloc()` / `kfree()`

无专用 cache 的通用请求 → **几何级数大小** 的通用 Slab cache（32B … 131072B）。

| 接口 | 作用 |
|------|------|
| **`kmalloc()`** | 内核小内存分配 |
| **`kfree()`** | 释放 |

驱动、网络栈大量路径依赖 kmalloc。

---

### 五、内存池 `mempool_t`

**极端内存紧张** 时：

- 预先 **储备** 一批对象  
- 关键路径 **不会因分配失败而阻塞**  

用于必须成功的内核路径（如某些 I/O 提交）。

→ Ch 5 信号量实例：Slab 链表保护 [section-7](../../chapter-05-kernel-synchronization/notes/section-7-选型与实例.md)

### 常见陷阱

1. 把 ULK 讲的 SLAB 当现代默认——SLUB 已取代 SLAB 成为默认，SLOB 用于嵌入式小内存
2. 混淆 SLAB/SLUB/SLOB——SLAB（复杂、per-CPU 队列）、SLUB（简化、性能好）、SLOB（极小、嵌入式）
3. 以为 `kmalloc()` 是唯一的内核分配器——还有 `vmalloc()`（虚拟连续）、`alloc_pages()`（页级）、`kmem_cache_alloc()`（专用 slab）

---

<details>
<summary>自测题（点击展开）</summary>

**Q1.** SLAB、SLUB、SLOB 三者的区别？为什么 SLUB 成为默认？

<details><summary>答案</summary>

SLAB：ULk 时代默认，复杂的多层 per-CPU 队列结构，管理开销大。SLUB（2.6.23+ 默认）：简化结构，每个 slab 只一个 per-CPU page，减少元数据开销，调试友好。SLOB：极简，用于内存极小的嵌入式系统（<16MB）。SLUB 胜出原因：① 更低元数据开销。② 更好的 NUMA 性能。③ 内联 freelist 简化。④ `slabinfo` 工具兼容。

</details>

**Q2.** `kmalloc()` / `vmalloc()` / `kmem_cache_alloc()` 的区别和选择？

<details><summary>答案</summary>

`kmalloc(size, flags)`：物理连续 + 虚拟连续（直接映射区），限制在 ~32MB（MAX_ORDER），快。`vmalloc(size)`：虚拟连续但物理不连续，可分配大块（GB 级），慢（需建页表 + TLB 压力）。`kmem_cache_alloc(cache, flags)`：从专用 slab cache 分配固定大小对象，最快，零内碎片。选择：小对象 → `kmem_cache_alloc`；通用小/中 → `kmalloc`；大块非连续 → `vmalloc`。

</details>

**Q3.** HFT 用户态如何实现类似 slab 的对象池？

<details><summary>答案</summary>

```c
// 预分配固定大小对象池
struct Pool {
    void *base;       // mmap 预分配
    size_t obj_size;
    size_t capacity;
    std::atomic<size_t> free_idx;
};
void* alloc(Pool* p) {
    size_t idx = p->free_idx.fetch_add(1, std::memory_order_relaxed);
    if (idx >= p->capacity) return nullptr;
    return (char*)p->base + idx * p->obj_size;
}
// 关键：无锁、预分配、cache-line 对齐
``` 优势：零分配延迟、无系统调用、无锁竞争。

</details>

</details>

---

← [2. 页框管理](./section-2-页框管理.md) · 下一节 [4. vmalloc](./section-4-非连续内存与vmalloc.md)
> ↔ [LKD Ch12 §12.7 Slab-层](../../../05-linux-kernel/chapter-12-memory-management/notes/section-12.7-Slab-层.md)
> ↔ [LKD Ch12 §12.5 kmalloc-与-kfree](../../../05-linux-kernel/chapter-12-memory-management/notes/section-12.5-kmalloc-与-kfree.md)
