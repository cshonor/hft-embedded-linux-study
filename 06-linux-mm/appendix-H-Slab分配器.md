# 附录 H Slab 分配器 · Slab Allocator

> **Code Commentary** · Mel Gorman · **选读** · 源码核验：Linux v6.6

概念总览 → [./chapter-08-slab-allocator/](./chapter-08-slab-allocator/)（现代默认 **SLUB**）

---

## 本节走读什么

原书附录 H 走读 **Slab**，但 v6.6 的默认分配器是 **SLUB**（`mm/slub.c`，164KB）+ 通用层（`mm/slab_common.c`）。本附录走读 **SLUB 的 fast/slow path** 与 **kmalloc 分档机制**。

---

## 1. SLUB 分配路径（fast/slow）

```
kmem_cache_alloc_node(s, gfp, node)          // slub.c:3521
        │
slab_alloc_node(s, lru, gfp, node, ...)      // slub.c:3453  fast path
        │  从 per-CPU freelist 直接 pop 一个对象（无锁、cmpxchg）
        ├─ 成功 → 返回对象
        │  失败（freelist 空）↓
        ▼
__slab_alloc_node(...)                       // slub.c:3329/3404
        │
___slab_alloc(...)                           // slub.c:3095  slow path
        │  关抢占 → 取新 slab → 或从 node partial 找 → 或 alloc_pages 新 slab
```

**走读要点**：SLUB 的核心是 **per-CPU freelist**——每个 CPU 缓存一个「空闲对象链表」，分配/释放都是**无锁的链表头 pop/push**（`cmpxchg_double` 原子操作）。只有 freelist 空（分配）或满（释放）时才 fallback 到 slow path 操作 slab 页。这就是 SLUB 比老 Slab 快的原因：**快路径不碰全局锁**。

## 2. 释放路径：`do_slab_free` + `slab_free_hook`

```
kmem_cache_free(s, x)
        │
do_slab_free(...)                            // 释放回 per-CPU freelist（无锁 push）
        │  freelist 满 → 归还 slab 页
        ▼
slab_free_hook(s, x)                         // slub.c:1766  KASAN/调试钩子
```

**走读要点**：释放和分配对称——优先回 per-CPU freelist，满了才 flush 回页。`slab_free_hook`（:1766）是调试/KASAN 的挂钩点，生产内核里通常编译为空。

## 3. kmalloc 分档：`kmalloc_caches`（slab_common.c:677）

`kmalloc(size)` 不是动态建 cache，而是**查表**落到预建的 `kmalloc_caches[][]`：

```c
kmalloc_caches[NR_KMALLOC_TYPES][KMALLOC_SHIFT_HIGH + 1]  // slab_common.c:677
    │  第一维 = 类型（normal/dma/cgroup/random）
    │  第二维 = 大小档（8/16/32/64... 2^n）
    ▼
kmalloc_slab(size, flags, caller)            // slab_common.c:728
        │  按 size 算 index → 返回对应 kmem_cache
```

**分档表** `kmalloc_info[]`（slab_common.c:824）定义了 8B ~ 8MB 的几十个档位（8/16/32/64/.../4K/8K/...）。**走读要点**：`kmalloc` 的大小必须对齐到 2 的幂档位，所以 `kmalloc(100)` 实际拿 128B 的 cache——这是 SLUB 与 Buddy 的衔接点：**小对象复用 cache，大对象（>8KB）直接转 `kmalloc_large` 走 page_alloc**。

## 4. shrinker：Slab 与回收的衔接

`struct shrinker`（include/linux/shrinker.h:63）让 slab cache 在内存压力下**收缩**：

| 字段 | 作用 |
|------|------|
| `count_objects` | 返回「可回收对象数」 |
| `scan_objects` | 实际回收 |
| `seeks` / `batch` | 回收代价权重 / 批量 |

**走读要点**：dcache、icache 等通过 shrinker 注册，Ch10 的 `shrink_slab` 在回收时回调它们——这就是「内存不足时先压缩 slab 缓存（dentry/inode）再换页」的机制入口。

---

## 与正文对应

| 附录内容 | 正文落点 |
|----------|----------|
| SLUB fast/slow path | Ch8（SLUB 机制） |
| per-CPU freelist | Ch8（快路径无锁） |
| `kmalloc_caches` 分档 | Ch8（kmalloc 分档）+ 05 模块 Ch12.7 |
| shrinker | Ch10 §4（收缩缓存） |

---

## HFT / 嵌入式关联

| 手段 | 落点 |
|------|------|
| 对象池 vs SLUB | 热路径小对象用**自定义对象池**（预先分配 + 无锁栈），避开 SLUB 的 slow path fallback |
| 对齐分档 | `kmalloc(100)` 拿 128B，若频繁分配 100B 会浪费 28B，考虑按 2 幂设计对象大小 |
| 观察收缩 | `/proc/slabinfo` 看 shrinker 是否频繁收缩 dcache/icache，判断内存压力 |

---

## 相关章节

- 上一章：[appendix-G-非连续内存分配.md](./appendix-G-非连续内存分配.md)
- 下一章：[appendix-I-高端内存管理.md](./appendix-I-高端内存管理.md)

---

<details>
<summary>自测 5 问（点开看答案）</summary>

**Q1：SLUB 的 fast path 为什么无锁？**

分配/释放都是操作 **per-CPU freelist**（空闲对象链表）的头，用 `cmpxchg_double` 原子 pop/push，不碰全局锁；只有 freelist 空/满才 fallback 到 slow path。

**Q2：`kmalloc(100)` 实际拿多大 cache？为什么？**

128B。因为 `kmalloc_caches` 按 2 的幂分档（`kmalloc_slab`，slab_common.c:728），大小必须对齐到档位。

**Q3：`kmalloc` 超过多大就不再走 slab、直接转 page_alloc？**

大于最大分档（8MB 级别）或超过 `KMALLOC_MAX_CACHE_SIZE`（通常 8KB）时，走 `kmalloc_large` 直接用 page_alloc 分配整页。

**Q4：shrinker 的 `count_objects` / `scan_objects` 分别干什么？**

`count_objects` 返回可回收对象数（用于决定是否值得收缩），`scan_objects` 执行实际回收（Ch10 `shrink_slab` 回调）。

**Q5：HFT 为什么常用自定义对象池而非依赖 SLUB？**

热路径小对象用预分配的无锁对象池，可完全避开 SLUB 的 slow path fallback（取新 slab / alloc_pages）导致的不可预期延迟。

</details>
