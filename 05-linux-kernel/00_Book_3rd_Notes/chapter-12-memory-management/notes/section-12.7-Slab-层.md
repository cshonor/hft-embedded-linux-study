## ⑦ Slab 层 · Slab Layer

内核 **大量固定大小对象**（`task_struct`、`inode`、`dentry`…）若每次走 **通用页分配器**，会 **慢** 且 **外部碎片** 严重 — **Slab 分配器** 在 **页之上** 做 **对象缓存**。

#### 三层结构（心智模型）

```
kmem_cache（一种对象类型）
    │
    ├── slab 1  [满]  无空槽
    ├── slab 2  [半满] ← 优先从这里 alloc
    └── slab 3  [空]   备用
            │
            └── 每个 slab = 若干连续页，切成 fixed-size 槽
```

| 概念 | 说明 |
|------|------|
| **Cache（`kmem_cache`）** | 一种 **对象类型** 一条缓存 — 统一 **构造/析构** |
| **Slab** | 一条 cache 内 **一页或多页** 的块 — **满 / 半满 / 空** 状态 |
| **对象（object）** | 实际 **`kmalloc` 大小的槽** — 带 **着色** 减 cache line 冲突 |

#### 分配策略

| 步骤 | 行为 |
|------|------|
| 1 | 找 **半满 slab** 的空槽 — **O(1) 常见** |
| 2 | 无半满 → 从 **空 slab** 或 **向 buddy 要新页** 建 slab |
| 3 | 释放 → 槽回 slab；全空 slab **可归还 buddy** |

#### 主要 API

| API | 作用 |
|-----|------|
| **`kmem_cache_create(name, size, align, flags, ctor)`** | 建 **专用 cache** |
| **`kmem_cache_destroy(cache)`** | 销毁（须 **无 live 对象**） |
| **`kmem_cache_alloc(cache, gfp)`** | 取对象 |
| **`kmem_cache_free(cache, obj)`** | 归还 |
| **通用 Slab** | **`kmalloc`** 内部选 **合适 size 的 general cache** |

```c
struct kmem_cache *my_cache;

my_cache = kmem_cache_create("my_obj", sizeof(struct my_obj),
                             0, SLAB_HWCACHE_ALIGN, NULL);
struct my_obj *o = kmem_cache_alloc(my_cache, GFP_KERNEL);
kmem_cache_free(my_cache, o);
```

#### Slab 变体（书外名词）

| 名称 | 特点 |
|------|------|
| **SLUB** | 现代默认 — **per-CPU partial list**，少锁 |
| **SLAB** | 经典三路 slab 链表 |
| **SLOB** | 嵌入式 **极简** — 省内存、慢 |

#### 与 `kmalloc` 栈

| 路径 | 说明 |
|------|------|
| **`kmalloc(128, gfp)`** | 命中 **`kmalloc-128`** general cache |
| **专用 cache** | 驱动 **固定结构** — 比裸 `kmalloc` **更可预测** |

**HFT：** 用户态 **typed object pool**（订单对象、事件 struct）= **`kmem_cache_*` 用户版**。内核 **网络栈 `sk_buff`** 等有 **专用 cache** — **NAPI poll** 路径 **复用 skb** 而非每次 `alloc_pages`。实盘：**池化 + 复用** 减 **allocator 锁竞争** 与 **TLB 抖动**。

→ [06 Gorman Ch8 Slab](../../../../06-linux-mm/chapter-08-slab-allocator/) · [Ch 3 task_struct Slab](../../chapter-03-process-management/) · [Ch 12.10 per-CPU](./section-12.10-每个-CPU-的分配.md)


> ↔ [ULK Ch8 §3 Slab分配器](../../../../20-linux-kernel-deep/chapter-08-memory-management/notes/section-3-Slab分配器.md)


<details>
<summary>自测题（点击展开）</summary>

**Q1.** Slab 分配器的三级缓存是什么？为什么能加速对象分配？

<details><summary>答案</summary>

Slab > Slub（现代默认）> Slob（嵌入式）。以 Slub 为例：每个 CPU 有 per-CPU partial 页，分配时从当前 CPU 的 partial 页上取空闲对象，无需锁、无需 buddy 调用。释放时放回 per-CPU partial。只有 partial 耗尽才向 buddy 申请新页。这就是为什么内核频繁分配/释放 task_struct 不会变慢。

</details>

**Q2.** kmalloc-128 和 kmalloc-256 是什么？为什么有这么多 slab cache？

<details><summary>答案</summary>

内核为每个 2 的幂大小（8/16/32/64/128/256/512/1024/2048/4096/8192）预创建专用 slab cache。kmalloc(100) 会在 kmalloc-128 中分配（向上取整到 128）。这样不同大小的对象不会互相碎片化，且每个 cache 的对象大小一致、对齐一致，cache 友好。

</details>

</details>
---
