# Ch 8 Slab分配器 · Slab Allocator

> **Understanding the Linux Virtual Memory Manager** · Mel Gorman · **精读**（HFT：**对象池 / per-CPU 本地缓存 / cache line 对齐** 的用户态设计，直接对照本章）

[Ch 6 Buddy](./chapter-06-物理页分配.md) 按 **整页（2^n）** 分配 — 快，但对 **小于一页** 的请求造成 **内部碎片**。**Slab 分配器** 在 **Buddy 之上** 做 **对象级缓存**：把 **物理页切成固定大小对象**，并 **复用已释放实例**。

> **时代说明：** 原书描述 **经典 SLAB**（`kmem_cache_t`、`slab_t`、`kmem_bufctl`）。**现代主线** 默认多为 **SLUB**（`mm/slub.c`）— API 仍是 **`kmem_cache_*` / `kmalloc`**，内部实现更简；**思想一致**：cache → slab(full/partial/free) → per-CPU 本地池。读源码以当前树为准（[`mm/slub.c`](https://elixir.bootlin.com/linux/latest/source/mm/slub.c) 或 `CONFIG_SLAB` 时 [`mm/slab.c`](https://elixir.bootlin.com/linux/latest/source/mm/slab.c)）。

---

## 本章在 VM 子系统中的位置

```
Ch 6 Buddy ──整页──►  Slab 切对象 / kmalloc 尺寸档
        ↑                    │
        └──── grow slab ─────┘（kmem_cache_grow → alloc_pages）
Ch 4 进程 mm_struct、inode 等 ──► 多由 slab 分配
HFT 用户态 ──► DPDK mempool / 自研 order pool 是同构设计
```

**HFT 为什么要精读：** 不是要写内核 slab，而是 **订单簿节点、Order 对象、fix message** — **固定大小 + 池化 + 每核缓存 + cache line 对齐**，与 slab 三大目标 **一一对应**。

→ 交叉：[Hennessy Ch2 cache line](../02-Computer-Architecture-6th/chapter-02-memory-hierarchy-design/) · [10-DPDK mempool](../14-DPDK-Low-Latency-Network/01-Intro-Book/notes/chapter-01-DPDK架构与EAL/)

---

## 1. Slab 分配器的三大核心目标

| 目标 | 机制 | HFT 用户态镜像 |
|------|------|----------------|
| **消除内部碎片** | 小于一页的请求 **按对象大小** 从 slab 切分，而非整页浪费 | **object pool** 只分配 `sizeof(Order)` 对齐块，不 `malloc` 任意大小 |
| **缓存常用对象** | 释放的对象 **保持已初始化状态** 挂在 cache 上，避免重复 ctor/dtor | **free list 复用 Order** — reset 字段而非 `new/delete` |
| **优化硬件缓存（Slab 着色）** | 用 slab **剩余空间作 color 偏移**，使 **不同 slab 上同类型对象** 落到 **不同 cache line**，减 **false sharing / 行冲突** | **`alignas(64)`**、按 core 分 arena、padding 热字段 |

### Slab 着色 (Colouring) — 简图

```
同一 kmem_cache（对象大小固定）
  Slab A:  [obj][obj][obj]…  + color_offset_0  → 对象起始地址 mod cache_line = α
  Slab B:  [obj][obj][obj]…  + color_offset_1  →  mod cache_line = β  (α≠β)
```

**目的：** 多 CPU 同时访问 **不同 slab** 上的 **同类型对象** 时，少 **挤在同一条 cache line**。

---

## 2. 核心数据结构：Cache 与 Slab

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

## 3. 对象分配与释放

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

## 4. 尺寸缓存 (Sizes Cache) 与 `kmalloc` / `kfree`

除 **类型专用 cache**（`kmem_cache_create("my_struct", …)`）外，内核大量 **任意小尺寸** 请求走 **通用尺寸 cache**：

| 特点 | 说明 |
|------|------|
| **2 的幂次档位** | 原书：**32 B ~ 128 KiB** 一系列 **`size-N`** cache |
| **两套** | 常规 + **`size-N(DMA)`** — DMA 可达物理区 |
| **API** | **`kmalloc(size, gfp)`** / **`kfree()`** — 按 size 选最近档位 cache |

**内部碎片仍存在：** `kmalloc(33)` 可能占 **64 B 档** — 比 Buddy 整页好，但 **档内浪费**。

**HFT：** 避免 **`malloc` 任意 size**；热路径 **固定 struct + 池** — 对应 **专用 kmem_cache** 而非泛型 kmalloc。

---

## 5. 每 CPU 对象缓存 (Per-CPU Object Cache)

| 问题 | 方案 |
|------|------|
| 多核抢 **cache->spinlock** | 每 CPU **`array_cache`** — **本地 object 栈** |
| 分配 / 释放 | **优先本地 batch**，**无锁** |
| 本地空 / 满 | **bulk** 与 **全局 slabs_partial / slabs_free** 交换 |

与 [Ch 6 pageset](./chapter-06-物理页分配.md#6-26-内核的新变化)（**页** 的 per-CPU）并列 — **Slab 做 object 级 per-CPU**。

**HFT 镜像：**

```
Core 0:  local Order[64]  ── pop/push 无锁
         空了 → 从 global pool 一次拿 32 个
Core 1:  同上
```

→ go-dex 若多 goroutine 抢全局 `sync.Pool` — 同类 **锁争用** 问题；**按 P 绑定 pool**（或 Rust thread-local）是 slab 思路。

---

## 6. 2.6 内核的新变化

### 内存池 `mempool`

**`mempool_t`** — 在 **系统极度缺内存** 时仍 **保留最低限度关键对象**（如 **bio、scsi 结构**）— **预分配 + reserve list**，**GFP 失败也不死**。

**HFT：** 交易 **reserve order slot**、**预分配 cancel 队列深度** — 同一 **「内存压力下仍要能完成关键路径」** 逻辑。

### 缓存回收：shrinker 取代 `kmem_cache_reap()`

| 2.4 及更早 | 2.6+ |
|------------|------|
| **`kmem_cache_reap()`** 盲目全局 reap | **`set_shrinker()`** — cache **注册自定义 shrink 回调** |
| 粗暴 | **按压力、按类型** 智能释放 **slabs_free** 回 Buddy |

现代 **`register_shrinker()`** / **`shrink_slab`** — 与 **kswapd / memcg** 联动。

---

## Buddy → Slab → kmalloc 一图

```
         应用 / 内核子系统
                │
    ┌───────────┼───────────┐
    │           │           │
 kmem_cache_alloc   kmalloc    get_free_pages
 (专用类型)      (任意小尺寸)   (整页)
    │           │           │
    └───────────┴───────────┘
                │
           Slab / SLUB
      full / partial / free
      per-CPU object cache
                │
           Buddy (Ch 6)
```

---

## HFT 精读 checklist

| Slab 概念 | 用户态落地 |
|-----------|------------|
| **专用 cache** | 一种消息/订单 **一种 pool** |
| **partial 优先** | 先 **pop free list**，空了再 **grow** |
| **constructor** | pool 分配时 **一次初始化**，复用时 **reset 热字段** |
| **着色 / 对齐** | **64B align**，热冷数据 **分 cache line** |
| **per-CPU array** | **每核 mempool**，批量 refill |
| **mempool reserve** | **预留 N 个 slot** 给 **风控/撤单** 关键路径 |
| **kmalloc 式泛型分配** | 热路径 **禁止** — 用 **固定 size pool** |

---

## 相关章节

- 上一章：[chapter-07-非连续内存分配.md](./chapter-07-非连续内存分配.md)
- 下一章：[chapter-09-高端内存管理.md](./chapter-09-高端内存管理.md)
- 内部碎片来源：[chapter-06-物理页分配.md](./chapter-06-物理页分配.md#5-避免碎片化-avoiding-fragmentation)
- 附录 H：[appendix-H-Slab分配器.md](./appendix-H-Slab分配器.md)

---
