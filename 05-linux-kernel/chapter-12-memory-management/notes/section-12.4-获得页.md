## ④ 获得页 · Getting Pages

当需要 **整页、物理连续**（DMA、页表、大块缓冲）时，走 **页分配器** — 在 **`kmalloc` 上限之上** 或 **Slab 之下** 的原始接口。

#### 核心 API

| API | 返回值 | 说明 |
|-----|--------|------|
| **`alloc_pages(gfp, order)`** | `struct page *` | **2^order** 个连续页 |
| **`__get_free_pages(gfp, order)`** | `unsigned long`（VA） | 页框 + **内核线性地址** |
| **`get_zeroed_page(gfp)`** | VA | **order 0** 且 **清零** — 防泄漏旧数据到用户态 |
| **`free_pages(addr, order)`** | — | 与 `__get_free_pages` 配对 |
| **`__free_pages(page, order)`** | — | 与 `alloc_pages` 配对 |

```c
/* 分配 8 页（order=3 @ 4KB/page = 32KB）连续物理 */
struct page *pg = alloc_pages(GFP_KERNEL, 3);
void *vaddr = page_address(pg);   /* 需 direct map 或已 kmap */

__free_pages(pg, 3);
```

#### `order` 与大小

| order | 页数 @ 4KB | 总大小 |
|-------|------------|--------|
| 0 | 1 | 4 KB |
| 3 | 8 | 32 KB |
| 8 | 256 | 1 MB |
| 10 | 1024 | 4 MB |

#### 快路径：per-CPU 页集合（pcp）

**order-0（单页）分配不走 `zone->lock`**，而是先打本 CPU 的私有页集合（v6.6 实证）：

```c
/* include/linux/mmzone.h:679 */
struct per_cpu_pages {
	spinlock_t lock;	/* Protects lists field */
	int count;		/* number of pages in the list */
	int high;		/* high watermark, emptying needed */
	int batch;		/* chunk size for buddy add/remove */
	short free_factor;	/* batch scaling factor during free */
#ifdef CONFIG_NUMA
	short expire;		/* When 0, remote pagesets are drained */
#endif
	/* Lists of pages, one per migrate type stored on the pcp-lists */
	struct list_head lists[NR_PCP_LISTS];
} ____cacheline_aligned_in_smp;
```

```
alloc_pages(GFP_KERNEL, 0)
   │
   ├─ 快路径 rmqueue_pcplist：只锁「本 CPU 的 pcp->lock」
   │     命中 → 取一个页，返回                     ← 无全局锁，几十纳秒级
   │     落空 → 一次向 buddy 拉 batch 个页进 pcp   ← 摊薄后续分配成本
   │
   └─ 慢路径：持 zone->lock 从 free_list 取，必要时拆分高阶块
```

释放是对称的：先放进 pcp，**`count > high` 时一次归还 `batch` 个**给伙伴系统。

> **三个实战含义**：
> ① `____cacheline_aligned_in_smp` —— pcp 结构体**按缓存行对齐**，避免本 CPU 的计数器和邻居 CPU 的互相伪共享（呼应 [19.4](../../chapter-19-portability/notes/section-19.4-数据对齐和结构体填充.md) 的 false sharing）；
> ② `batch` 是**批量摊薄**的关键：一次锁 zone 换 N 个页，把全局锁的开销除以 N；
> ③ **跨 CPU 释放有代价**——页回到**释放者**的 pcp，而不是分配者的。
> 高频"CPU A 分配、CPU B 释放"会让两侧 pcp 反复触及 high 水位、频繁批量归还/借出。

#### 慢路径：四级降级

```
① pcp（本 CPU，无全局锁）
     ↓ 空
② 伙伴系统（持 zone->lock，拆分高阶块）
     ↓ 该 zone 水位不足
③ 回收与整理：唤醒 kswapd → 直接回收（direct reclaim）→ compaction 迁移整理
     ↓ 仍失败
④ OOM killer（或返回 NULL，取决于 gfp 是否允许重试）
```

> **步骤 ③ 是延迟毛刺的主要来源**：直接回收可能要**等待脏页写回**（磁盘 I/O），
> compaction 要**迁移页面并冻结页表**。所以 **`GFP_KERNEL` 在中断上下文/自旋锁内是禁用的**
> ——它可能睡眠，且睡眠时长**不可预测**。

#### `gfp_mask`：v6.6 定义已拆到 `gfp_types.h`

⚠️ 版本变化：`include/linux/gfp.h` 现在**只有函数声明和辅助函数**，
所有标志定义搬到了 **`include/linux/gfp_types.h`**。查标志别找错文件。

常用组合（`gfp_types.h:325-333` 实证，逐字）：

```c
#define GFP_ATOMIC	(__GFP_HIGH|__GFP_KSWAPD_RECLAIM)
#define GFP_KERNEL	(__GFP_RECLAIM | __GFP_IO | __GFP_FS)
#define GFP_KERNEL_ACCOUNT (GFP_KERNEL | __GFP_ACCOUNT)
#define GFP_NOWAIT	(__GFP_KSWAPD_RECLAIM)
#define GFP_NOIO	(__GFP_RECLAIM)
#define GFP_NOFS	(__GFP_RECLAIM | __GFP_IO)
#define GFP_USER	(__GFP_RECLAIM | __GFP_IO | __GFP_FS | __GFP_HARDWALL)
#define GFP_DMA		__GFP_DMA
#define GFP_DMA32	__GFP_DMA32
```

| 组合 | 能睡眠？ | 能回收？ | 能 I/O？ | 能用紧急储备？ | 典型场景 |
|------|---------|---------|---------|--------------|---------|
| `GFP_KERNEL` | ✅ | ✅ | ✅ | ❌ | 进程上下文的默认选择 |
| `GFP_ATOMIC` | ❌ | ❌（只唤醒 kswapd） | ❌ | ✅ `__GFP_HIGH` | 中断/自旋锁内 |
| `GFP_NOWAIT` | ❌ | ❌（只唤醒 kswapd） | ❌ | ❌ | 不想睡眠，也不该动用储备 |
| `GFP_NOIO` | ✅ | ✅ | ❌ | ❌ | 块设备/存储栈内，防止回收递归触发 I/O |
| `GFP_NOFS` | ✅ | ✅ | ✅ 但禁文件系统 | ❌ | 文件系统内，防止回收递归进 FS |

> **最容易被说错的一条**：`GFP_ATOMIC` **不等于"完全不回收"**——
> 它是 `__GFP_HIGH | __GFP_KSWAPD_RECLAIM`，即**能碰紧急储备**且**会唤醒 kswapd**，
> 只是**不会自己直接回收**（因为直接回收可能睡眠）。
> 真正"什么都不做、试一次就走"的是 `GFP_NOWAIT`。

**失败语义三档**（重试力度从强到弱）：

| 标志 | 行为 |
|------|------|
| **默认** | 尽全力：回收 + compaction + 可能触发 OOM |
| **`__GFP_RETRY_MAYFAIL`** | 会重试，但**放弃 OOM**，且不重试 compaction |
| **`__GFP_NORETRY`** | **一次失败即返回 NULL**，不做激进尝试（`gfp_types.h:180` 有详细说明） |

| 标志 | 页分配行为 |
|------|------------|
| **`GFP_KERNEL`** | 可 **睡眠** 等内存、触发回收 |
| **`GFP_ATOMIC`** | **不睡眠** — 仅从 reserve / 紧急池 取，**易失败** |
| **`__GFP_ZERO`** | 分配后清零 |
| **`__GFP_HIGHMEM`** | 允许 HIGHMEM 页（须 kmap 访问） |
| **`__GFP_NORETRY`** | 失败 **不重试** 更激进路径 |
| **`__GFP_NOWARN`** | 失败时**不打印**分配失败告警（自己能处理失败的路径应加） |

#### 与 `kmalloc` 的关系

| 事实 | 说明 |
|------|------|
| **`kmalloc` 内部** | ≤ 8KB 从 **Slab/SLUB** 缓存；**> 8KB 直接 `alloc_pages`** |
| **`KMALLOC_MAX_SIZE`** | v6.6 = **4MB**（由 `MAX_ORDER=10` 决定） — 再大须 **`vmalloc` 或页 API** |

##### v6.6 实证的分界线

```c
/* include/linux/slab.h（SLUB 分支） */
#define KMALLOC_SHIFT_HIGH	(PAGE_SHIFT + 1)              /* 13 */
#define KMALLOC_SHIFT_MAX	(MAX_ORDER + PAGE_SHIFT)      /* 10 + 12 = 22 */
#define KMALLOC_MAX_SIZE	(1UL << KMALLOC_SHIFT_MAX)        /* 4 MB */
#define KMALLOC_MAX_CACHE_SIZE	(1UL << KMALLOC_SHIFT_HIGH)   /* 8 KB */
```

| 请求大小 | 实际走哪条路 |
|---------|------------|
| **≤ 8KB** | SLUB 缓存（`kmalloc_caches[]` 按 size index 分档） |
| **> 8KB** | **`kmalloc_large()` → 直接页分配器**（`slab.h:595`：`if (size > KMALLOC_MAX_CACHE_SIZE) return kmalloc_large(size, flags);`） |
| **> 4MB** | `kmalloc` 直接失败 |

> 也就是说 **`kmalloc(64KB)` 本质上就是 `alloc_pages(order=4)`**，
> 只是多了"按大小选 order + 记账"的包装。
> **"slab 碎片"只对 ≤8KB 的请求成立**；超过 8KB 之后你面对的是**伙伴系统的碎片**问题。

#### 释放纪律

| 错误 | 后果 |
|------|------|
| **漏 `free_pages`** | **永久泄漏** — 该页框永不回 buddy |
| **double free** | 破坏 buddy 链表 — **内存腐败** |
| **order 不匹配** | **BUG** — 必须与分配时 order 相同 |

**HFT：** 网卡驱动 **RX ring** 的 **descriptor + 包缓冲** 常用 **页分配或 dma_pool** — 启动时 **一次性** `alloc_pages`，运行期 **GFP_ATOMIC 零分配**。用户态 **hugepage 池** 同理：**启动预占**，盘中 **不再向内核要连续 2MB**。

> **HFT 补充：把"分配失败"当正常分支处理。**
> 慢路径可能走到直接回收与 compaction，**耗时不可预测**——在热路径上就是尾延迟。三条硬规则：
> ① **热路径零分配**：RX/TX ring、订单簿槽位在启动/建连阶段一次分配好，数据路径只读写；
> ② 中断与自旋锁内**只能 `GFP_ATOMIC`**，且必须**处理返回 NULL**（它本来就容易失败）；
> ③ 能优雅降级的路径加 **`__GFP_NOWARN`**，避免一次可预期的失败刷满 dmesg
> （dmesg 有全局锁，高频打印会污染**所有** CPU 的延迟）。

→ [06 Gorman Ch6 物理页分配](../../../06-linux-mm/chapter-06-physical-page-allocation/) · [Ch 12.5 kmalloc](./section-12.5-kmalloc-与-kfree.md)



<details>
<summary>自测题（点击展开）</summary>

**Q1.** alloc_pages(gfp, order) 中 order 是什么？最大能分配多少？

<details><summary>答案</summary>

order 是 2 的幂次方页数。alloc_pages(gfp, 0) = 1 页(4KB)，order=1 = 2 页(8KB)... v6.6 的 `MAX_ORDER` 默认 = **10**（`mmzone.h:28`，`!CONFIG_ARCH_FORCE_MAX_ORDER` 时）= 1024 页 = **4MB**。超过 4MB 需用 vmalloc 或 CMA。HFT 内核驱动分配大块 DMA buffer 用 alloc_pages 而非 kmalloc（避免 slab 碎片）。

> **订正**：常见说法「MAX_ORDER 通常 = 10 或 11」在 v6.6 是**确定的 10**（v6.5 也是 10）。
> 架构可用 `CONFIG_ARCH_FORCE_MAX_ORDER` 覆盖，但改大只会让**高阶分配更容易失败**
> ——需要的连续页数翻倍了。

</details>

**Q2.** __get_free_pages 和 alloc_pages 的关系？

<details><summary>答案</summary>

__get_free_pages(gfp, order) = alloc_pages(gfp, order) + page_address()。即分配页并返回虚拟地址。但只能用于 ZONE_NORMAL/ZONE_DMA（有直接映射地址），HIGHMEM 页用 kmap 获取地址。现代内核推荐直接用 alloc_pages + page_address。

</details>

**Q3.** order-0 的页分配为什么不抢 `zone->lock`？per-CPU pageset 是怎么工作的？

<details><summary>答案</summary>

v6.6 `mmzone.h:679` 的 `struct per_cpu_pages`（pcp）为每个 CPU、每个 zone 维护私有页链表（按迁移类型分 `lists[NR_PCP_LISTS]`，结构本身 `____cacheline_aligned_in_smp` 防伪共享）。分配 order-0 先走 `rmqueue_pcplist`，只锁**本 CPU 的 `pcp->lock`**；落空才一次向伙伴系统批量拉 `batch` 个页。释放对称：先放入 pcp，`count > high` 时一次归还 `batch` 个给 buddy。用**批量摊薄**把全局 `zone->lock` 的开销除以 batch。两个后果：① 高频「CPU A 分配、CPU B 释放」会让两侧 pcp 频繁触及 high 水位；② `batch` 会随 `free_factor` 缩放，NUMA 上还有 `expire` 定期排空远端 pageset。

</details>

**Q4.** `GFP_ATOMIC` 真的完全不做回收吗？它和 `GFP_NOWAIT` 有什么区别？

<details><summary>答案</summary>

不完全是。v6.6 `gfp_types.h:325` 实证：`#define GFP_ATOMIC (__GFP_HIGH|__GFP_KSWAPD_RECLAIM)`——它能**动用紧急储备**（`__GFP_HIGH`）且**会唤醒 kswapd**（`__GFP_KSWAPD_RECLAIM`），只是**不会自己直接回收**（直接回收可能睡眠）。而 `GFP_NOWAIT` 只有 `__GFP_KSWAPD_RECLAIM`，**既不能睡眠也不能碰紧急储备**。所以：需要「不睡眠但希望尽力拿到内存」用 GFP_ATOMIC（中断/自旋锁内）；需要「试一下就走、不许动用储备」用 GFP_NOWAIT。两者都必须处理返回 NULL。

</details>

**Q5.** `kmalloc(64KB)` 到底走了哪条路？

<details><summary>答案</summary>

走**页分配器**，不走 slab。v6.6 `slab.h` 实证：`KMALLOC_MAX_CACHE_SIZE = 1UL << (PAGE_SHIFT + 1)` = **8KB**，而 `kmalloc()` 的实现里有 `if (size > KMALLOC_MAX_CACHE_SIZE) return kmalloc_large(size, flags);`。所以 ≤8KB 走 SLUB 的 `kmalloc_caches[]` 分档缓存，>8KB 直接 `alloc_pages()`（本质是按大小算出 order）。推论：**「slab 碎片」只对 ≤8KB 的请求成立**，超过 8KB 面对的是伙伴系统的碎片问题；而 `kmalloc` 的绝对上限 `KMALLOC_MAX_SIZE = 1UL << (MAX_ORDER + PAGE_SHIFT)` = 4MB。

</details>

</details>
---
