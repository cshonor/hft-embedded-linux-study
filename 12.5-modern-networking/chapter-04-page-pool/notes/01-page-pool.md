# 01 — page_pool 机制本体：Rx buffer 的回收池

> **内核文档：** `Documentation/networking/page_pool.rst`
> **对应 Rosen:** Ch1（Rx buffer 分配）
> **内核版本:** 以 **v6.6** 为准，API 签名与注释均取自
> `include/net/page_pool/types.h`、`include/net/page_pool/helpers.h`、`net/core/page_pool.c`

## 文档概述

page_pool 是现代 Linux 收包路径的**内存底座**。它不显眼，但决定了两件事：

1. 高 PPS 下收包还能不能撑住（`alloc_page()` 在 1 Mpps 以上会成为瓶颈）
2. **XDP 能不能用**（XDP 依赖 page_pool 的回收语义，没有 page_pool 就没有 XDP）

本篇讲 page_pool 的**机制本体**——它内部怎么组织、分配/释放的判定规则是什么。

本篇与兄弟篇的分工：

| 篇 | 讲什么 |
|----|--------|
| **01（本篇）** | 机制本体：三层结构、分配/释放 API、**refcnt 判定规则**、frag 模式 |
| [02-page-pool-lwn](02-page-pool-lwn.md) | 为什么需要它、驱动采用情况、**HFT 调优清单**、陷阱 |
| [chapter-03/02](../../chapter-03-tx-path-skbbuff/notes/02-txrx.md) | 传统 vs 现代驱动的对照 |
| [chapter-03/04](../../chapter-03-tx-path-skbbuff/notes/04-sk-buff-xdp-buff.md) | page_pool 的页如何零拷贝变成 skb |

原笔记只有 1.4 KB，列了 5 个参数、5 步流程、一行性能对比。下面补齐的是**回收判定规则**——那才是 page_pool 真正的机制核心，也是所有 page_pool 相关 bug 的来源。

---

## 一、问题：`alloc_page()` 在收包路径上贵在哪

每个 Rx buffer 都是一页（或半页）。传统路径上，每收一个包就要：

| 步骤 | 代价 |
|------|------|
| `alloc_page()` → 伙伴系统 | 拿 zone->lock（**全局竞争**）、查 free list、可能触发回收 |
| `dma_map_page()` | 建 IOMMU 映射、刷 TLB（有 IOMMU 时更贵） |
| 页面引用计数原子操作 | cacheline 在多个 CPU 间弹 |
| 释放时 `dma_unmap_page()` + `put_page()` | 再来一遍，同样有锁 |

**关键不是单次多贵，而是这些操作每包都要做两遍（分配 + 释放），且都带全局竞争。** 在 1 Mpps 下，单个 CPU 每秒要做 200 万次伙伴系统往返——这个量级上，`alloc_page()` 本身就能吃掉收包预算的一大半。

page_pool 的思路很朴素：**页面用完别还给伙伴系统，放回自己的池子里。**

---

## 二、三层结构

```
        ┌─────────────────────────────────────────────┐
        │  ① per-CPU cache（无锁，最快）                │
        │     pool->alloc.cache[]                      │
        │     命中 → 统计量 rx_pp_alloc_fast            │
        └──────────────────┬──────────────────────────┘
                           │ miss
                           v
        ┌─────────────────────────────────────────────┐
        │  ② ptr_ring（跨 CPU / NAPI 之间周转）          │
        │     命中 → 统计量 rx_pp_alloc_slow            │
        └──────────────────┬──────────────────────────┘
                           │ 空
                           v
        ┌─────────────────────────────────────────────┐
        │  ③ 伙伴系统（真正的 alloc_page，慢路径）        │
        │     统计量 rx_pp_alloc_empty / _refill        │
        └─────────────────────────────────────────────┘

回收方向（对称）：
  释放 → refcnt == 1 ? → ① per-CPU cache（rx_pp_recycle_cached）
                       → ① 满 → ② ptr_ring（rx_pp_recycle_ring）
                       → ② 满 → ③ 还给伙伴系统（rx_pp_recycle_released_ref）
```

这个三层结构不是猜的——它正好对应 v6.6 `net/core/page_pool.c` 里导出的 **11 个统计量**：

```c
static const char pp_stats[][ETH_GSTRING_LEN] = {
	"rx_pp_alloc_fast",        /* ① per-CPU 缓存命中 */
	"rx_pp_alloc_slow",        /* ② ptr_ring 命中 */
	"rx_pp_alloc_slow_ho",     /* ② 高 order 页 */
	"rx_pp_alloc_empty",       /* ③ 池空，走伙伴系统 */
	"rx_pp_alloc_refill",      /* ③ 正在补充 */
	"rx_pp_alloc_waive",       /* ③ 放弃填充 */
	"rx_pp_recycle_cached",    /* 回收进 ① */
	"rx_pp_recycle_cache_full",/* ① 满 */
	"rx_pp_recycle_ring",      /* 回收进 ② */
	"rx_pp_recycle_ring_full", /* ② 满 */
	"rx_pp_recycle_released_ref", /* refcnt>1，真释放 */
};
```

**这组数字就是 page_pool 的健康体检表**，见第五节。

---

## 三、分配：两个 API，选错会浪费内存

```c
/* include/net/page_pool/helpers.h */

static inline struct page *page_pool_dev_alloc_pages(struct page_pool *pool)
{
	gfp_t gfp = (GFP_ATOMIC | __GFP_NOWARN);
	return page_pool_alloc_pages(pool, gfp);
}

static inline struct page *page_pool_dev_alloc_frag(struct page_pool *pool,
						    unsigned int *offset,
						    unsigned int size)
{
	gfp_t gfp = (GFP_ATOMIC | __GFP_NOWARN);
	return page_pool_alloc_frag(pool, offset, size, gfp);
}
```

| API | 返回 | 用在哪 |
|-----|------|--------|
| `page_pool_dev_alloc_pages()` | **一整页** | 大帧、jumbo frame；或不在意内存浪费的简单驱动 |
| `page_pool_dev_alloc_frag()` | 一页里的**一段**（通过 `offset` 返回偏移） | **小包场景**，配合 `PP_FLAG_PAGE_FRAG` |

注意两者的 gfp 都是 **`GFP_ATOMIC | __GFP_NOWARN`**——收包路径不能睡眠。

### frag 模式：一页切给多个包

4 KB 一页给一个 64 字节的包，浪费 98%。frag 模式把一页切成多段，每包占一段。

相关的计数器操作：

```c
static inline void page_pool_fragment_page(struct page *page, long nr);
static inline long page_pool_defrag_page(struct page *page, long nr);

static inline bool page_pool_is_last_frag(struct page_pool *pool,
					  struct page *page)
{
	/* If fragments aren't enabled or count is 0 we were the last user */
	return !(pool->p.flags & PP_FLAG_PAGE_FRAG) ||
	       (page_pool_defrag_page(page, 1) == 0);
}
```

**为什么不能用 page 的 refcount 来管 frag？** 内核源码里那句注释说明了：

> *"pp_frag_count represents the number of writers who can update the page
> either by updating `skb->data` or via DMA mappings for the device.
> We can't rely on the page refcnt for that as we don't know who might be…"*

即：page 的 refcount 是"被谁持有"，而 `pp_frag_count` 是"谁可能写这块内存"。**语义不同，必须分开计。** 混淆这两者是 page_pool 相关 bug 的经典来源。

---

## 四、释放：refcnt 决定"回收"还是"真释放"

**这是 page_pool 最核心的一条规则**，直接看 `page_pool_put_page()` 的官方注释：

```c
/**
 * page_pool_put_page() - release a reference to a page pool page
 * @pool:	pool from which page was allocated
 * @page:	page to release a reference on
 * @dma_sync_size: how much of the page may have been touched by the device
 * @allow_direct: released by the consumer, allow lockless caching
 *
 * The outcome of this depends on the page refcnt. If the driver bumps
 * the refcnt > 1 this will unmap the page. If the page refcnt is 1
 * the allocator owns the page and will try to recycle it in one of the pool
 * caches. If PP_FLAG_DMA_SYNC_DEV is set, the page will be synced for_device
 * using dma_sync_single_range_for_device().
 */
```

翻成判定表：

| 释放时 page refcnt | 归属 | 结果 |
|------------------|------|------|
| **== 1** | 池仍独占该页 | **回收**进 per-CPU cache 或 ptr_ring |
| **> 1** | 有人（协议栈 / clone / 转发）还拿着 | **unmap 并真释放**回伙伴系统 |

**这条规则的实践含义**：

- XDP_DROP / 驱动丢弃 → refcnt 仍是 1 → **直接回收，零成本**
- XDP_PASS → 页变成 skb 的数据区，refcnt 可能被抬高 → skb 释放时如果还有别人持有，就**回不了池**
- GRO 合并、包被 clone、转发到其他设备 → 都会抬高 refcnt → 页面"漏出"pool

所以**页面回不了池不是 page_pool 的失职，是设计如此**：只要还有人在用这块内存，就不能回收。这也是为什么高 PPS 下要盯 `rx_pp_recycle_released_ref`——它涨了，说明你的页面在往外漏。

### 三个释放 API 的区别

```c
static inline void page_pool_put_page(struct page_pool *pool,
				      struct page *page,
				      unsigned int dma_sync_size,
				      bool allow_direct);

static inline void page_pool_put_full_page(struct page_pool *pool,
					   struct page *page, bool allow_direct)
{
	page_pool_put_page(pool, page, -1, allow_direct);
}

static inline void page_pool_recycle_direct(struct page_pool *pool,
					    struct page *page)
{
	page_pool_put_full_page(pool, page, true);
}
```

| API | `dma_sync_size` | `allow_direct` | 语义 |
|-----|-----------------|----------------|------|
| `page_pool_put_page()` | 调用者指定同步多少字节 | 调用者定 | 通用 |
| `page_pool_put_full_page()` | `-1`（按 `params.max_len` 全同步） | 调用者定 | 常用 |
| `page_pool_recycle_direct()` | `-1` | **`true`** | **调用者必须保证安全上下文（如 NAPI），页直接进 per-CPU 快缓存** |

`page_pool_recycle_direct()` 的注释很硬：

> *"Similar to `page_pool_put_full_page()` but caller must guarantee safe context
> (e.g NAPI), since it will recycle the page directly into the pool fast cache."*

**这是最快的释放路径**（XDP_DROP 就走它），但用错上下文会破坏 per-CPU 缓存的无锁假设。

### DMA 地址

```c
static inline dma_addr_t page_pool_get_dma_addr(struct page *page)
{
	dma_addr_t ret = page->dma_addr;
	if (PAGE_POOL_DMA_USE_PP_FRAG_COUNT)
		ret |= (dma_addr_t)page->dma_addr_upper << 16 << 16;
	...
}
```

注意那个条件：当 `sizeof(dma_addr_t) > sizeof(unsigned long)` 时，DMA 地址**被拆成两个字段存**（`dma_addr` + `dma_addr_upper`）。这是 32 位系统配 64 位 DMA 地址的情形。写驱动时别直接读 `page->dma_addr`，要用这个函数。

---

## 五、flags 与创建参数（v6.6 完整版）

### 三个 flag

```c
#define PP_FLAG_DMA_MAP		BIT(0) /* Should page_pool do the DMA map */
#define PP_FLAG_DMA_SYNC_DEV	BIT(1) /* If set all pages that the driver gets
					* will be DMA-synced for_device */
#define PP_FLAG_PAGE_FRAG	BIT(2) /* for page frag feature */
#define PP_FLAG_ALL		(PP_FLAG_DMA_MAP | PP_FLAG_DMA_SYNC_DEV | PP_FLAG_PAGE_FRAG)
```

| flag | 作用 | 不开会怎样 |
|------|------|-----------|
| `PP_FLAG_DMA_MAP` | 让 pool 做 DMA 映射，并**保持映射**（页面在池期间一直有效） | 驱动要自己每次 map/unmap |
| `PP_FLAG_DMA_SYNC_DEV` | 每次分配页面时做一次 `dma_sync_for_device` | 驱动要自己 sync（否则可能读到脏 cache） |
| `PP_FLAG_PAGE_FRAG` | 启用一页多段 | 每个包占一整页，内存浪费 |

**`PP_FLAG_DMA_MAP` 是 page_pool 最大的一笔性能收益**：它让 DMA 映射**只做一次、长期保持**，收发循环里完全省掉 `dma_map`/`dma_unmap` 这一对（有 IOMMU 时每次可达数百 ns）。

### 创建参数（v6.6）

```c
/* include/net/page_pool/types.h */
struct page_pool_params {
	unsigned int	flags;
	unsigned int	order;
	unsigned int	pool_size;
	int		nid;
	struct device	*dev;
	struct napi_struct *napi;
	enum dma_data_direction dma_dir;
	unsigned int	max_len;
	unsigned int	offset;
/* private: used by test code only */
	void (*init_callback)(struct page *page, void *arg);
	void *init_arg;
};
```

| 参数 | 说明 | HFT 关注 |
|------|------|---------|
| `flags` | 上面三个 flag 的组合 | 必须带 `PP_FLAG_DMA_MAP` |
| `order` | 页阶（0 = 4 KB，1 = 8 KB…） | 0 最常用；大帧考虑 1 |
| `pool_size` | 池的初始/环形容量 | 见 [02 篇](02-page-pool-lwn.md) 的调优清单 |
| `nid` | **NUMA 节点** | ⚠️ 必须设成网卡所在的 node |
| `dev` | 关联的 `struct device`（DMA 用） | — |
| `napi` | 关联的 `napi_struct` | 影响 `allow_direct` 的判定 |
| `dma_dir` | DMA 方向，收包是 `DMA_FROM_DEVICE` | — |
| `max_len` | **`PP_FLAG_DMA_SYNC_DEV` 时**的最大同步长度 | 不设会导致 sync 不全 |
| `offset` | DMA 同步地址的偏移 | 配合 `max_len` |
| `init_callback` / `init_arg` | 仅测试代码用 | 生产别碰 |

> ⚠️ 常见资料里只列 5 个参数（`pool_size`/`order`/`flags`/`nid`/`dma_dir`），
> v6.6 实际有 **10 个**。`max_len` / `offset` / `napi` 都是后来加的，
> `max_len` 与 `PP_FLAG_DMA_SYNC_DEV` 配套，漏设会造成同步范围错误。

### NUMA 节点可以运行时改变

```c
static inline void page_pool_nid_changed(struct page_pool *pool, int new_nid);
```

存在这个 API 本身就说明一件事：**网卡的 NUMA 节点不是一成不变的**（热插拔、驱动重绑、某些虚拟设备迁移）。如果你的系统有这类情况，需要注意 pool 的 `nid` 可能与网卡实际位置脱节。

---

## 六、与 XDP / AF_XDP 的关系

| XDP 动作 | page 去向 |
|---------|----------|
| `XDP_DROP` | `page_pool_recycle_direct()` 直接回池，**零分配零释放** |
| `XDP_PASS` | 页变成 skb 的数据区（`napi_build_skb()`），**不复制** |
| `XDP_TX` | 页复用做发送，完成后回收 |
| `XDP_REDIRECT`（cpumap/devmap） | 页随 `xdp_frame` 走，`mem` 字段记着"该还给哪个 pool" |
| `XDP_REDIRECT`（AF_XDP 零拷贝） | 页来自用户态 UMEM，**不是 page_pool** |

最后一行很重要：AF_XDP 零拷贝模式下内存是 `MEM_TYPE_XSK_BUFF_POOL`（用户态 UMEM），**与 page_pool 是两套体系**。很多资料说"AF_XDP 基于 page_pool"，准确说法是：**AF_XDP 的 copy 模式用 page_pool，零拷贝模式不用。**

> 详见 [chapter-06 AF_XDP](../../chapter-06-af-xdp/) 与
> [chapter-03/04](../../chapter-03-tx-path-skbbuff/notes/04-sk-buff-xdp-buff.md)。

---

## 七、观测

### ⚠️ 先更正一个常见说法：`/proc/net/page_pools` 在 v6.6 主线**不存在**

我核对了 v6.6 的 `net/core/page_pool.c`，**全文没有任何 `proc_create` / `proc_net` 注册**。
很多笔记（包括本仓库之前的版本）写的 `cat /proc/net/page_pools` 在标准 v6.6 上是空文件或不存在的
——如果你在自己的机器上看到了，那是发行版补丁或别的版本。**先确认再依赖。**

### 真正的观测出口：ethtool 统计接口

v6.6 提供的是一组 ethtool stats 回调：

```c
int  page_pool_ethtool_stats_get_count(void);
u8  *page_pool_ethtool_stats_get_strings(u8 *data);
u64 *page_pool_ethtool_stats_get(u64 *data, void *stats);

bool page_pool_get_stats(struct page_pool *pool, struct page_pool_stats *stats);
```

`page_pool_get_stats()` 的注释明确说了条件：

> *"This API is only available if the kernel has been configured with
> `CONFIG_PAGE_POOL_STATS=y`. A pointer to a caller allocated struct
> `page_pool_stats` structure is passed to this API which is filled in.
> The caller can then report those stats to the user (perhaps via ethtool, debugfs, etc.)."*

所以正确姿势是：

```bash
# 1) 确认内核开启了统计
grep PAGE_POOL_STATS /boot/config-$(uname -r)
#   或 zcat /proc/config.gz | grep PAGE_POOL_STATS

# 2) 通过 ethtool 看（驱动需集成 page_pool 统计）
ethtool -S eth0 | grep -i pp_

# 3) 先看有没有 /proc/net/page_pools（多数机器没有）
ls -l /proc/net/page_pools 2>/dev/null || echo "（v6.6 主线无此文件）"
```

### 11 个统计量怎么读

| 统计量 | 含义 | 健康信号 |
|--------|------|---------|
| `rx_pp_alloc_fast` | per-CPU 缓存命中 | **这个应该占绝大多数** |
| `rx_pp_alloc_slow` | 走了 ptr_ring | 偏高说明本 CPU 缓存不够 |
| `rx_pp_alloc_slow_ho` | 高 order 页 | 与 `order` 设置有关 |
| `rx_pp_alloc_empty` | 池空，走伙伴系统 | **持续非 0 = 池太小** |
| `rx_pp_alloc_refill` | 正在补充 | — |
| `rx_pp_alloc_waive` | 放弃填充 | — |
| `rx_pp_recycle_cached` | 回收进 per-CPU 缓存 | 应该占绝大多数 |
| `rx_pp_recycle_cache_full` | 缓存满 | — |
| `rx_pp_recycle_ring` | 回收进 ptr_ring | — |
| `rx_pp_recycle_ring_full` | ring 满 | — |
| **`rx_pp_recycle_released_ref`** | **refcnt > 1，真释放回伙伴系统** | **★ 持续上涨 = 页面在漏出池** |

**核心诊断**：`rx_pp_alloc_fast / (总分配)` 越接近 1 越好；`rx_pp_recycle_released_ref` 越大说明越多页面回不了池（GRO 合并、clone、转发都会导致）。后者涨到一定程度，page_pool 就退化成"每次都 alloc_page"了。

---

## HFT 要点

- **page_pool 的最大收益是 `PP_FLAG_DMA_MAP` 的保持映射**，不是"避免 alloc_page"。有 IOMMU 时每次 map/unmap 可达数百 ns，这个才是大头。
- **`rx_pp_recycle_released_ref` 是最该盯的数字**：它涨说明页面回不了池，page_pool 正在失去意义。
- **`page_pool_recycle_direct()` 最快但要求调用者在 NAPI 上下文**——XDP_DROP 走的就是这条。用错上下文会破坏无锁假设。
- **frag 模式对行情小包是刚需**：4 KB 页给 64 字节的包浪费 98%，而且浪费的是 cache 和 TLB 覆盖，不只是内存。
- **`pp_frag_count` 和 page refcount 是两套计数**：前者管"谁能写"，后者管"谁持有"。混用会出难以复现的内存 bug。
- **`nid` 必须设成网卡所在 NUMA 节点**，不是当前线程所在节点。这个坑和 DPDK 的 `rte_eth_dev_socket_id()` 是同一类。
- **AF_XDP 零拷贝不用 page_pool**（用用户态 UMEM）。说"AF_XDP 基于 page_pool"只在 copy 模式下成立。
- **`/proc/net/page_pools` 在 v6.6 主线不存在**，别把它写进运维手册。

## 与 Rosen 3.x 的差异

| Rosen 3.x（2.6/3.x 时代） | 现在（5.x/6.x） |
|--------------------------|----------------|
| Rx buffer 每次 `alloc_page()` | page_pool 池化复用 |
| DMA 每次 map / unmap | `PP_FLAG_DMA_MAP` 保持映射，**只做一次** |
| 一页一包 | `PP_FLAG_PAGE_FRAG` 一页多段 |
| 无 XDP，也就不需要回收语义 | **XDP 依赖 page_pool 的"refcnt==1 就回收"规则** |
| 无 page_pool 统计 | 11 个 `rx_pp_*` 统计量（需 `CONFIG_PAGE_POOL_STATS=y`） |
| NUMA 靠驱动自己管 | `page_pool_params.nid` 显式指定 + `page_pool_nid_changed()` |

**方法论上的变化**：Rosen 时代的 Rx buffer 是"用完就还"的租借模型；page_pool 是"**只要没人用就留在自己手里**"的所有制模型。判定依据是 refcnt——这个判定规则是 page_pool 的全部精髓。

---

## 代码自测

<details>
<summary>Q1：你启用了 page_pool，但 <code>rx_pp_alloc_empty</code> 持续上涨，<code>rx_pp_recycle_released_ref</code> 也在涨。怎么定位？</summary>

<b>答：</b>两个数字同时涨，指向同一个根因：<b>页面回收不回来，池被抽干</b>。

按这条链查：

1. **`rx_pp_recycle_released_ref` 为什么会涨？** 回顾 `page_pool_put_page()` 的规则——
   释放时 refcnt > 1 就 unmap 并真释放回伙伴系统。所以一定有人在抬高 refcnt。

2. **谁在抬高 refcnt？** 常见的几个：
   - **GRO**：合并后的 skb 持有多个页片段
   - **包被 clone**：`skb_clone()` 共享数据区
   - **转发 / bridge / bond**：`rx_handler` 之后包去了别的设备
   - **AF_PACKET（tcpdump）**：抓包会让 skb 被克隆一份给 tap
   - **应用没及时读**：skb 在 socket 接收队列里堆积，页一直被占

3. **快速验证**：`bpftrace` 挂在 `page_pool_put_defragged_page` 上看 refcnt 分布，
   或者直接对照——关掉 GRO、停掉 tcpdump，看 `released_ref` 是否掉下来。

4. **处置方向**：
   - 增大 `pool_size`（治标）
   - 减小 GRO 合并窗口（`gro_flush_timeout`，见 chapter-02/04）
   - 排障期间别开 tcpdump（它会 clone，本身就是干扰源）
   - 确认应用读取速度跟得上（`/proc/net/udp` 的 rx_queue）

<b>注意</b>：`released_ref` 涨不是 page_pool 的 bug，是设计如此——还有人在用，就不能收。
</details>

<details>
<summary>Q2：<code>page_pool_recycle_direct()</code> 和 <code>page_pool_put_full_page(pool, page, true)</code> 完全等价吗？</summary>

<b>答：</b>从代码上看是等价的——

```c
static inline void page_pool_recycle_direct(struct page_pool *pool,
					    struct page *page)
{
	page_pool_put_full_page(pool, page, true);
}
```

但<b>语义上不等价</b>，差别在调用契约。`page_pool_recycle_direct()` 的注释：

> *"Similar to `page_pool_put_full_page()` but caller must guarantee safe context
> (e.g NAPI), since it will recycle the page directly into the pool fast cache."*

它明确要求调用者<b>保证自己在 NAPI 上下文</b>，因为 `allow_direct = true` 会让页面直接进
per-CPU 缓存——而那个缓存是<b>无锁</b>的，只在"本 CPU 不会并发访问"的前提下安全。

所以：
- 在驱动的 `napi_poll()` 里（XDP_DROP 路径）→ 用 `page_pool_recycle_direct()`，最快
- 在中断上下文、或其他不确定的上下文 → 用 `page_pool_put_full_page(pool, page, false)`，
  让它走 ptr_ring，安全但慢一点

<b>直接写 `put_full_page(pool, page, true)` 不算错，但把契约埋起来了</b>——
用 `page_pool_recycle_direct()` 这个显式的名字，等于把"我在 NAPI 里"这个前提写进代码。
</details>

<details>
<summary>Q3：你给 page_pool 设了 <code>PP_FLAG_DMA_SYNC_DEV</code>，但没设 <code>max_len</code>。会出什么问题？</summary>

<b>答：</b>`max_len` 与 `PP_FLAG_DMA_SYNC_DEV` 是配套的，看 `page_pool_params` 的字段注释：

```
 * @max_len:	max DMA sync memory size for PP_FLAG_DMA_SYNC_DEV
 * @offset:	DMA sync address offset for PP_FLAG_DMA_SYNC_DEV
```

`PP_FLAG_DMA_SYNC_DEV` 的行为是：每次驱动拿到页面时，为 device 做一次 DMA 同步
（`dma_sync_single_range_for_device`）。同步<b>多少字节</b>就由 `max_len` 和 `offset` 决定。

不设（默认 0）的实际后果是<b>同步范围不对</b>——该同步的区域可能没被同步。在有
non-coherent DMA 的架构上（部分 ARM、以及带 IOMMU 且 cache 不一致的场景），
表现为<b>读到脏数据或旧数据</b>，而且是<b>间歇性</b>的：取决于 cacheline 恰好有没有被写过。

这类 bug 极难查，因为：
- 大部分 x86 服务器是 cache-coherent 的，同步是 no-op，所以测试环境往往复现不了
- 一旦换到 non-coherent 平台就随机出错

<b>结论</b>：如果开了 `PP_FLAG_DMA_SYNC_DEV`，必须同时设 `max_len`（通常是 MTU + headroom + tailroom），
有偏移就设 `offset`。不确定就<b>别开这个 flag</b>——让驱动自己显式 sync，范围自己清楚。
</details>

---

→ 本篇：[01 page_pool 机制本体](01-page-pool.md)
→ 后一篇：[02 page_pool：为什么需要它 + HFT 调优清单](02-page-pool-lwn.md)
→ 相关：[chapter-03/02 驱动契约](../../chapter-03-tx-path-skbbuff/notes/02-txrx.md) · [chapter-05 XDP](../../chapter-05-xdp-architecture/) · [chapter-06 AF_XDP](../../chapter-06-af-xdp/)
