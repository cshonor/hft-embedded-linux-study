# 内存分配 (SLAB → SLUB / folio)

> 笨叔《奔跑吧 Linux 内核》读书笔记
> 对应旧书: ULK3 / LKD3 (Linux 2.6)
> 对应现代内核: Linux 5.x / 6.x

---

## 本节要点

### 物理内存分配的三层架构

Linux 内存分配遵循**三层架构**：伙伴系统（页级）→ Slab/SLUB（对象级）→ vmalloc（非连续物理页）。

1. **伙伴系统 (Buddy System)**：管理物理页帧（page frame），按 order 0-10 分配 2^order 页。每个 zone 有独立的 free_area[]。
2. **Slab/SLUB 分配器**：在伙伴系统之上，为频繁分配/释放的同类型对象（如 task_struct、inode、sk_buff）提供高效的对象池。
3. **vmalloc**：分配物理不连续但虚拟连续的内存，用于大块内核映射。

### SLAB → SLUB 迁移

| 特性 | SLAB (2.6) | SLUB (2.6.23+, 默认) | SLUB 现代改进 (5.x/6.x) |
|------|-----------|---------------------|------------------------|
| 每 CPU 缓存 | struct array_cache | struct kmem_cache_cpu | 同左，但 per-CPU partial 列表改进 |
| 空闲对象追踪 | 复杂的 per-node 数组 | 简化的 freelist 指针 | 同左 |
| 调试支持 | 内建 | 通过 CONFIG_SLUB_DEBUG | dynamic debug 支持 |
| 合并相似 cache | 不支持 | 支持（sysfs 合并控制） | 支持 + sysfs 可关闭 |
| NUMA | 复杂 | 简化 | per-node partial 改进 |

**关键变化：** SLUB 在 2.6.23 成为默认，SLAB 在 6.1 被完全移除。现代内核只保留 SLUB 和 SLOB（嵌入式精简版）。

### folio API（5.x+ 引入，6.x 成熟）

`folio` 是对 `page` 的包装，解决了历史上 `page` 混用 compound page 和 base page 的歧义问题。

```c
// 旧 API (page-based, 易出错)
struct page *page = alloc_pages(gfp, order);
void *addr = page_address(page);  // 如果是 compound page, 返回头页地址

// 新 API (folio-based, 类型安全)
struct folio *folio = folio_alloc(gfp, order);
void *addr = folio_address(folio);  // 明确是 folio 级别操作
size_t size = folio_size(folio);    // 直接返回 folio 大小 (order 0 = 4KB, order 1 = 8KB...)
```

### 关键分配函数

```c
// 伙伴系统 (页级)
struct page *alloc_pages(gfp_t gfp, unsigned int order);
struct folio *folio_alloc(gfp_t gfp, unsigned int order);  // 新 API
void __free_pages(struct page *page, unsigned int order);

// SLUB (对象级)
void *kmalloc(size_t size, gfp_t flags);
void *kmalloc_node(size_t size, gfp_t flags, int node);  // NUMA 指定节点
void kfree(const void *objp);

// vmalloc (非连续)
void *vmalloc(unsigned long size);
void *vzalloc(unsigned long size);
void vfree(const void *addr);
```

---

## 与旧书对比

| ULK3 / LKD3 (2.6) | 笨叔 (5.x/6.x) | 变化原因 |
|--------------------|-----------------|----------|
| SLAB 是默认分配器 | SLUB 是默认，SLAB 已移除 (6.1) | SLUB 更简单、NUMA 更好、调试更灵活 |
| `struct page` 用于一切 | `struct folio` 用于复合页操作 | page 歧义导致 bug 难追踪 |
| `alloc_pages()` 返回 page | `folio_alloc()` 返回 folio | 类型安全，编译器能检查 |
| 无 per-CPU partial 优化 | SLUB per-CPU partial 列表 | 减少 zone lock 竞争 |
| bootmem 启动分配器 | memblock 取代 bootmem (3.9+) | memblock 更简洁 |

---

## 关键数据结构 / 函数

```c
// 源码路径: mm/slub.c
struct kmem_cache {
    struct kmem_cache_cpu __percpu *cpu_slab;  // per-CPU 缓存
    slab_flags_t flags;
    unsigned long min_partial;
    unsigned int size;           // object 大小 (含 metadata)
    unsigned int inuse;          // 实际可用大小
    unsigned int obj_offset;     // metadata 偏移
    int object_size;
    struct kmem_cache_node *node[MAX_NUMNODES];  // per-node partial
};

// 源码路径: include/linux/mm_types.h
struct folio {
    struct page page;  // 包装 page
    // ... folio 特有字段
};

// 源码路径: mm/page_alloc.c (伙伴系统)
struct zone {
    struct free_area free_area[MAX_ORDER + 1];  // order 0-10
    // ...
};
```

---

## HFT 关联

- **SLUB per-CPU 缓存**：HFT 交易线程在同一 CPU 上反复分配/释放 sk_buff，命中 per-CPU 缓存无需锁，延迟可低至 ~20ns
- **kmalloc 对齐**：HFT 自定义数据结构用 `kmalloc` 分配，SLUB 保证对象对齐（通常 64B），避免 cache line 跨界
- **大页 (HugePages)**：减少 TLB miss，HFT 常用 `mmap(MAP_HUGETLB)` 分配 2MB 大页
- **NUMA 绑定**：`kmalloc_node()` 确保分配在交易线程所在 NUMA 节点，避免跨节点访问延迟（~100-300ns 额外）

---

## 自测

<details>
<summary>Q1: SLUB 相比 SLAB 的主要优势是什么？为什么 SLAB 在 6.1 被移除？</summary>

SLUB 的优势：(1) 代码更简洁（SLAB 有复杂的 per-node array_cache 管理）；(2) NUMA 扩展性更好（per-CPU partial 列表减少锁竞争）；(3) 调试更灵活（CONFIG_SLUB_DEBUG 可运行时开关）；(4) 相似大小的 cache 可合并，减少内存碎片。SLAB 在 6.1 被移除是因为 SLUB 已成为默认超过 15 年，SLAB 无人维护，代码冗余。
</details>

<details>
<summary>Q2: folio 和 page 的区别？为什么引入 folio？</summary>

`page` 结构体既表示单页也表示 compound page 的头页/尾页，调用者必须自己判断是否是 compound page，容易出错。`folio` 明确表示"一个完整的内存对象"（可能是单页或复合页），编译器可以类型检查。例如 `folio_size(folio)` 直接返回大小，而 `PAGE_SIZE << compound_order(page)` 需要先检查是否是 compound head。
</details>

<details>
<summary>Q3: HFT 场景下 kmalloc 和 vmalloc 应如何选择？</summary>

优先用 `kmalloc`：(1) 物理连续，TLB 友好；(2) 命中 SLUB per-CPU 缓存时无锁，~20ns；(3) DMA 友好。只有在需要大块虚拟连续内存（>4MB）且物理连续性不要求时才用 `vmalloc`。HFT 的订单簿、行情数据用 `kmalloc` + 大页；日志缓冲区等非延迟敏感的大块用 `vmalloc`。
</details>
