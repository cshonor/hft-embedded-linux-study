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

→ Ch 5 信号量实例：Slab 链表保护 [section-7](../chapter-05-kernel-synchronization/notes/section-7-选型与实例.md)

---

← [2. 页框管理](./section-2-页框管理.md) · 下一节 [4. vmalloc](./section-4-非连续内存与vmalloc.md)
