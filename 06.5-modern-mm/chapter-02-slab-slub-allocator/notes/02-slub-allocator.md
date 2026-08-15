# SLUB 分配器

> **原文:** [SLUB: The unqueued slab allocator](https://lwn.net/Articles/229096/) (LWN, 2007)
> **作者:** Christoph Lameter
> **内核版本:** 2.6.23+ (默认分配器)
> **对标旧书:** ULK3 Ch8 / LKD3 Ch12 (SLAB 描述)

---

## 核心观点

SLUB 是 Christoph Lameter 设计的 SLAB 替代方案，目标是**简化代码、改善 NUMA 扩展性、减少元数据开销**。

### SLAB 的问题

SLAB 维护复杂的 per-CPU array_cache 和 per-node 共享数组：
- 每个 CPU 有一个 array_cache，存放刚释放的对象（快路径无锁）
- 每个 node 有一个 shared array_cache，用于 CPU 间共享
- 结构复杂，调试困难，内存开销大

### SLUB 的设计

| 特性 | SLAB | SLUB |
|------|------|------|
| per-CPU 缓存 | array_cache (对象指针数组) | 单个 slab 页 + freelist 指针 |
| 空闲对象追踪 | 数组索引 | 内嵌在对象本身 (freelist pointer) |
| partial slab 管理 | per-node 3 个链表 | per-CPU partial + per-node partial |
| 元数据 | 外部管理 | 内嵌 (object 自带 next 指针) |
| 代码行数 | ~5000 行 | ~3000 行 |

### SLUB 的快路径

```c
// SLUB 快路径 (无锁)
// 源码路径: mm/slub.c
static inline void *slab_alloc_node(struct kmem_cache *s, gfp_t gfpflags,
                                    int node, unsigned long addr)
{
    struct kmem_cache_cpu *c = raw_cpu_ptr(s->cpu_slab);
    void *object;

    // 1. 检查 per-CPU freelist 是否有空闲对象
    object = c->freelist;
    if (likely(object && node_match(c, node))) {
        // 2. 快速分配: 移动 freelist 指针
        c->freelist = get_freepointer(s, object);
        return object;  // 无锁完成!
    }
    // 3. 慢路径: 从 partial slab 补充
    return __slab_alloc(s, gfpflags, node, addr, c);
}
```

---

## 与旧书差异

| ULK3 / LKD3 讲的 | 现代实现 | 差异 |
|-------------------|---------|------|
| SLAB 的 array_cache 结构 | SLUB 的 per-CPU freelist | 完全不同的设计 |
| `kmem_cache_t` 类型名 | `struct kmem_cache` | 类型名变更 |
| 3 条 partial 链表 | per-CPU + per-node partial | 简化 |
| 无 cache 合并 | SLUB 支持相似大小 cache 合并 | 减少碎片 |

---

## HFT 关联

SLUB 的 per-CPU freelist 快路径是 HFT 的关键性能路径：同一 CPU 上反复 `kmalloc/kfree` 同大小对象（如 sk_buff），快路径仅需一次指针移动，~20ns 完成。

---

<details>
<summary>自测题（点击展开）</summary>

**Q1:** SLUB 为什么叫 "unqueued" slab allocator？

> SLAB 用 array_cache 队列管理 per-CPU 对象。SLUB 不用队列，而是在每个空闲对象内嵌入 next 指针形成隐式链表，per-CPU 只保存一个 freelist 头指针。这消除了 array_cache 的元数据开销。

**Q2:** SLUB 的 cache 合并机制是什么？有什么好处和风险？

> 当两个 kmem_cache 的对象大小、对齐、flags 相同时，SLUB 自动合并为一个。好处：减少 slab 页数量，降低碎片。风险：合并后 slab 类型信息丢失，某些调试场景（如 KASAN 报告）难以区分对象来源。可通过 `slub_nomerge` 禁用。

</details>
