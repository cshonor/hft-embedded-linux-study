# 02 — page_pool 实践：为什么需要它、HFT 调优清单与陷阱

> **对应 Rosen:** Ch1/Ch4（收包路径中 buffer 分配）
> **内核版本:** page_pool 4.18+ 引入，广泛采用于 5.x 驱动；本文以 **v6.6** 为准
> **源码:** `net/core/page_pool.c`、`include/net/page_pool/types.h`

## 文档概述

上一篇 [01-page-pool](01-page-pool.md) 讲的是 **page_pool 内部怎么运作**（三层结构、refcnt 判定、分配/释放 API）。本篇讲**工程实践**：

1. 它到底解决了什么问题（性能账怎么算）
2. 容量模型与**调优清单**
3. 五个真实的陷阱
4. 怎么确认你的驱动真的在用

原笔记 2.2 KB，给了示例代码和一张驱动采用表。本篇保留那些，并补上原笔记完全没覆盖的部分：**容量模型**（`pool_size` 到底管什么）、**调优清单**、**陷阱**。

---

## 一、为什么需要 page_pool：三笔账

page_pool 的收益常被笼统地说成"避免 `alloc_page`"。拆开算其实是**三笔独立的账**：

| # | 省掉什么 | 为什么贵 | 由哪个机制省 |
|---|---------|---------|-------------|
| 1 | **伙伴系统往返** | `alloc_page()` 要拿 `zone->lock`，多核下是**全局竞争点**；`put_page()` 同样 | per-CPU 缓存 + ptr_ring 复用 |
| 2 | **DMA map / unmap** | 建 IOMMU 映射、刷 TLB，有 IOMMU 时每次可达数百 ns | **`PP_FLAG_DMA_MAP` 保持映射，只做一次** |
| 3 | **引用计数原子操作** | 每次 get/put 都是 atomic，cacheline 在多核间弹 | 池独占时（refcnt == 1）走无锁快路径 |

**第 2 笔往往是大头**，但最容易被忽略——很多资料只讲第 1 笔。如果你的机器开了 IOMMU（虚拟化环境、`intel_iommu=on`），`dma_map_page()` 的代价能到几百 ns，而 page_pool 把它**完全消除**（映射一次，长期保持）。

> ⚠️ 关于具体数字：流传很广的对比是"`alloc_page` ~300 cycles/包 → page_pool ~20 cycles/包"。
> 这个量级方向是对的，但**绝对值高度依赖**是否有 IOMMU、`order`、是否 frag 模式、
> per-CPU 缓存命中率。**别把它当基准，自己测**（方法见第六节）。

### 一个必要的澄清：`pool_size` 不管快缓存

这是最容易搞错的地方。看 v6.6 的源码常量：

```c
/* include/net/page_pool/types.h */
#define PP_ALLOC_CACHE_SIZE	128     /* per-CPU 缓存数组大小 */
#define PP_ALLOC_CACHE_REFILL	64      /* 批量补充的批量大小 */

struct page_pool {
	...
	struct {
		struct page *cache[PP_ALLOC_CACHE_SIZE];   /* ← 固定 128，不可配 */
		...
	} alloc;
	...
};
```

而 `pool_size` 管的是**第二层的 ptr_ring**：

```c
/* net/core/page_pool.c: page_pool_init() */
unsigned int ring_qsize = 1024; /* Default */
...
if (pool->p.pool_size)
	ring_qsize = pool->p.pool_size;

/* Sanity limit mem that can be pinned down */
if (ring_qsize > 32768)
	return -E2BIG;

...
if (ptr_ring_init(&pool->ring, ring_qsize, GFP_KERNEL) < 0)
	...
```

所以：

| 参数 | 控制什么 | 默认值 | 可调范围 |
|------|---------|--------|---------|
| per-CPU 缓存 | **固定 128**，硬编码 | 128 | ❌ **不可调** |
| 批量补充 | 固定 64，硬编码 | 64 | ❌ 不可调 |
| `pool_size` | **ptr_ring** 大小 | **1024** | ✅ 可调，**上限 32768**（超出返回 `-E2BIG`） |

**含义**：把 `pool_size` 从 1024 加到 8192，**不会让单个 CPU 的快路径变快**（快缓存永远是 128）。它变大的是"跨 CPU / NAPI 之间周转"的缓冲深度。所以调 `pool_size` 解决的是**突发和跨核归还**，不是单核分配延迟。

---

## 二、容量模型

```
  每个 CPU 的热页面（无锁，最快）
    ├─ per-CPU cache：128 个（固定）
    └─ 批量补充：一次 64 个
                    ↑
                    │ 溢出 / 跨 CPU 归还
                    v
  ptr_ring：pool_size 个（默认 1024，上限 32768）
                    ↑
                    │ 溢出
                    v
  真正释放回伙伴系统（rx_pp_recycle_released_ref / ring_full）

  外加：Rx ring 描述符占用的页面（不在池里，在网卡手里）
  外加：协议栈 / 应用手里持有的页面（refcnt > 1，回不来）
```

**估算总量时的三条经验**：

1. **在途页面** ≥ Rx ring 描述符总数（`ethtool -g eth0` 的 RX 值 × 队列数）。这部分一直在网卡手里，不在池里。
2. **per-CPU 缓存** = 128 × 参与收包的 CPU 数（硬开销，省不掉）。
3. **ptr_ring** 要能吸收突发：行情开盘那一瞬间可能有几万个包涌进来，`pool_size` 太小就会看到 `rx_pp_alloc_empty` 上涨。

**保守公式**：

```
pool_size ≥ 2 × (RX_DESC × QUEUES)
```

再往上调到 `rx_pp_alloc_empty` 归零即可。**注意上限 32768**——别写个 100 万进去，会直接 `-E2BIG` 创建失败。

---

## 三、驱动采用情况与确认方法

原笔记列了几个使用 page_pool 的驱动。这里保留，并给出**自己确认的方法**（比背一张表可靠）：

| 驱动 | 采用 page_pool | 备注 |
|------|---------------|------|
| mlx5（Mellanox/NVIDIA） | 是 | 5.x+，也是 page_pool 的主要推动方 |
| ice（Intel E810） | 是 | 5.x+ |
| stmmac（树莓派 5 网卡） | 是 | 5.x+，eBPF 真机实验常用的廉价平台 |
| virtio-net | 是 | 5.x+ |

**⚠️ 这张表会过时**（驱动随时可能改）。确认你自己机器上的情况，用：

```bash
# 方法 1：看内核配置
grep PAGE_POOL /boot/config-$(uname -r)

# 方法 2：看 ethtool 是否吐出 rx_pp_* 统计（需要 CONFIG_PAGE_POOL_STATS=y）
ethtool -S eth0 | grep -i "rx_pp_"
#   有输出 → 驱动在用 page_pool 且统计已开启
#   无输出 → 要么没用，要么统计没编译进去

# 方法 3：看驱动源码（最可靠）
#   在内核源码树里 grep 对应驱动文件
grep -l "page_pool" drivers/net/ethernet/<vendor>/*.c
```

---

## 四、HFT 调优清单

| 项 | 建议 | 理由 |
|----|------|------|
| `flags` | **必带 `PP_FLAG_DMA_MAP`** | 保持 DMA 映射，这是第二笔大账 |
| `flags` | 小包场景带 `PP_FLAG_PAGE_FRAG` | 一页多段，4 KB 给 64 B 包浪费 98% |
| `flags` | `PP_FLAG_DMA_SYNC_DEV` **慎开** | 开了必须同时设 `max_len`/`offset`，否则同步范围错误（见 [01 篇 Q3](01-page-pool.md)） |
| `nid` | **设成网卡所在 NUMA 节点** | ⚠️ 不是当前线程所在节点。与 DPDK `rte_eth_dev_socket_id()` 同一类坑 |
| `dma_dir` | 收包 `DMA_FROM_DEVICE`；**要用 XDP_TX 就必须是 `DMA_BIDIRECTIONAL`** | 源码里对 `PP_FLAG_DMA_MAP` 有校验，只允许这两个值 |
| `order` | 0（4 KB）起步；jumbo frame 考虑 1 | order 越大单页越大，frag 模式下的段数也越多 |
| `pool_size` | `≥ 2 × (RX_DESC × QUEUES)`，上限 32768 | 调它吸收突发，不改善单核分配延迟 |
| `napi` | 填上关联的 `napi_struct` | 影响 `allow_direct` 快路径的判定 |

### XDP_TX 与 `dma_dir` 的联动

源码注释说得很明确：

```c
/* DMA direction is either DMA_FROM_DEVICE or DMA_BIDIRECTIONAL.
 * DMA_BIDIRECTIONAL is for allowing page used for DMA sending,
 * which is the XDP_TX use-case.
 */
```

**要做 XDP_TX（原路反弹发送）就必须用 `DMA_BIDIRECTIONAL`**，否则映射方向不对。这一条在写 XDP 转发程序时经常漏。

---

## 五、五个陷阱

### 陷阱 1：别对 page_pool 的页面做 `get_page()` / `put_page()`

回收判定完全依赖 **refcnt == 1**（见 [01 篇](01-page-pool.md) 第四节）。你手动 `get_page()` 会把 refcnt 抬到 2，于是：

- 释放时走 **`rx_pp_recycle_released_ref`** 分支——**页面被真释放回伙伴系统**，永远离开池子
- page_pool 悄悄退化成"每次都 alloc_page"

**症状**：`rx_pp_recycle_released_ref` 持续上涨，性能逐渐劣化，但功能完全正常（不报错）。

### 陷阱 2：GRO / clone / tcpdump 会让页面回不了池

和陷阱 1 同源，但更隐蔽——**不是你干的**：

| 行为 | 抬高 refcnt 吗 | 后果 |
|------|---------------|------|
| GRO 合并 | 是 | 合并期间页面被多个片段持有 |
| `skb_clone()` | 是（共享数据区） | 克隆存活期间页面回不来 |
| **tcpdump / AF_PACKET** | **是**（给 tap 克隆一份） | **排障工具本身在干扰被测系统** |
| bridge / bond 转发 | 是 | 包去了别的设备 |
| 应用读取慢，skb 堆在接收队列 | 是 | 队列积压越久，页面被占越久 |

⚠️ **最后两行对 HFT 尤其重要**：
- 你在抓包排障时，tcpdump 正在改变系统的内存行为
- 应用读取跟不上时，不只是延迟升高——**页面被占住导致 page_pool 被抽干**，进而拖累整个收包路径

### 陷阱 3：驱动 reload 会重建 pool，页面全部丢失

```bash
ethtool -G eth0 rx 4096     # 改 ring 大小
ethtool -L eth0 combined 8  # 改队列数
rmmod/modprobe              # 重载驱动
```

这些操作会**销毁并重建 page_pool**，池里所有页面归还伙伴系统。之后的一段时间里：

- 每个包都要走"池空 → 伙伴系统"的慢路径
- `rx_pp_alloc_empty` 暴涨
- **表现为 reload 后的一段时间内延迟明显升高**，然后逐渐恢复

**对 HFT 的含义**：别在运行中改这些参数。要改就在开盘前改，改完等性能稳定再接入。

### 陷阱 4：per-CPU 缓存溢出是静默的

```c
static bool page_pool_recycle_in_cache(struct page *page,
				       struct page_pool *pool)
{
	if (unlikely(pool->alloc.count == PP_ALLOC_CACHE_SIZE)) {
		recycle_stat_inc(pool, cache_full);
		return false;      /* ← 静默降级到 ptr_ring */
	}
	/* Caller MUST have verified/know (page_ref_count(page) == 1) */
	pool->alloc.cache[pool->alloc.count++] = page;
	recycle_stat_inc(pool, cached);
	return true;
}
```

缓存满了就 **静默降级**到 ptr_ring，不报错、不告警。你只能通过 `rx_pp_recycle_cache_full` 统计量看到。

注意那句注释 **"Caller MUST have verified/know (page_ref_count(page) == 1)"**——调用 `page_pool_recycle_in_cache()` 之前必须已经确认 refcnt 为 1。这也是为什么 `page_pool_recycle_direct()` 要求调用者在 NAPI 上下文：**契约是靠调用者维护的，不是靠函数自己检查的。**

### 陷阱 5：统计默认可能没编译进去

`page_pool_get_stats()` 的注释：

> *"This API is only available if the kernel has been configured with
> `CONFIG_PAGE_POOL_STATS=y`."*

很多发行版内核**默认不开**这个配置。所以你 `ethtool -S` 看不到 `rx_pp_*`，不一定代表驱动没用 page_pool——也可能只是统计没编译进去。

```bash
grep PAGE_POOL_STATS /boot/config-$(uname -r)
#   没有输出 → 统计不可用，只能靠间接指标（如 /proc/meminfo、延迟变化）判断
```

---

## 六、观测与自测方法

```bash
# 1) 先确认统计是否可用
grep PAGE_POOL_STATS /boot/config-$(uname -r)

# 2) 核心健康指标
ethtool -S eth0 | grep -i "rx_pp_"
#   rx_pp_alloc_fast          应占总分配的绝大多数
#   rx_pp_alloc_empty         持续非 0 → pool_size 太小
#   rx_pp_recycle_cached      应占回收的绝大多数
#   rx_pp_recycle_released_ref 持续上涨 → 页面在漏出（见陷阱 1/2）

# 3) Rx ring 大小（估算 pool_size 用）
ethtool -g eth0

# 4) 网卡在哪个 NUMA 节点（设 nid 用）
cat /sys/class/net/eth0/device/numa_node
#   ⚠️ 返回 -1 表示该设备不提供 NUMA 信息（常见于部分虚拟设备）

# 5) 自己测 page_pool 的收益（别信网上的绝对值）
#    方法：开关 XDP 前后对比 rx_pp_alloc_fast 比例 + 用
#    chapter-15 的延迟测量方法对比 PPS 极限
```

**测"page_pool 有没有起作用"的最直接办法**：看 `rx_pp_alloc_fast` 占比。
如果它占总分配的 99% 以上，说明几乎全走 per-CPU 快缓存，page_pool 在正常工作。
如果 `rx_pp_alloc_empty` 或 `alloc_slow` 占比很高，说明池在被反复抽干。

---

## HFT 要点

- **最大的一笔收益是 `PP_FLAG_DMA_MAP` 的保持映射**，尤其在你开了 IOMMU 时。别只盯着 `alloc_page`。
- **`pool_size` 管的是 ptr_ring，不是快缓存**：快缓存固定 128，不可调。调 `pool_size` 吸收的是突发和跨核归还，不改善单核分配延迟。
- **`pool_size` 上限 32768**，超了直接创建失败（`-E2BIG`）。
- **XDP_TX 必须用 `DMA_BIDIRECTIONAL`**，否则映射方向不对。
- **tcpdump 会抬高 refcnt**，让页面回不了池——排障工具本身在干扰被测系统。
- **应用读取慢会抽干 page_pool**：不只是延迟问题，会通过 refcnt 传导到内存层。
- **运行中别改 ring 大小/队列数**：reload 会重建 pool，页面全丢，之后一段时间延迟明显升高。
- **`nid` 用网卡的 NUMA 节点**，不用当前线程的。
- **统计默认可能没编译**（`CONFIG_PAGE_POOL_STATS`），看不到 `rx_pp_*` 不代表 page_pool 没在工作。

## 与 Rosen 3.x 的差异

| Rosen 3.x | 现在（5.x/6.x） |
|-----------|----------------|
| Rx buffer: `alloc_page()` 每包一次 | page_pool 池化：per-CPU 缓存(128) → ptr_ring(默认 1024) → 伙伴系统 |
| DMA 每次 map/unmap | `PP_FLAG_DMA_MAP` 映射一次长期保持 |
| 一页一包 | `PP_FLAG_PAGE_FRAG` 一页多段 |
| 无回收判定概念 | **refcnt == 1 就回收，> 1 就真释放** |
| 内存 NUMA 靠驱动自己管 | `page_pool_params.nid` + 运行时 `page_pool_nid_changed()` |
| 无 page_pool 统计 | 11 个 `rx_pp_*`（需 `CONFIG_PAGE_POOL_STATS=y`） |
| XDP 不存在 | XDP 依赖 page_pool；XDP_TX 还要求 `DMA_BIDIRECTIONAL` |

---

## 代码自测

<details>
<summary>Q1：你把 <code>pool_size</code> 从 1024 调到了 8192，期待单核收包延迟下降，结果没变。为什么？</summary>

<b>答：</b>因为 <code>pool_size</code> 管的不是快缓存。

v6.6 源码里：

```c
#define PP_ALLOC_CACHE_SIZE	128     /* 固定，硬编码 */
...
unsigned int ring_qsize = 1024; /* Default */
if (pool->p.pool_size)
	ring_qsize = pool->p.pool_size;
...
ptr_ring_init(&pool->ring, ring_qsize, ...)
```

`pool_size` → `ring_qsize` → **ptr_ring 的大小**。而真正决定单核分配速度的是
**per-CPU 缓存（`cache[128]`，硬编码，不可配）**。

所以调 `pool_size` 改变的是"跨 CPU / 跨 NAPI 归还时能缓冲多少页面"，即：
- 突发吸收能力（行情开盘那一瞬）
- 跨核归还时的争用

它**不改善单核的分配延迟**——那是 128 个缓存槽的事，你动不了。

要验证调得对不对，看 `rx_pp_alloc_empty` 和 `rx_pp_recycle_ring_full`：
- 这两个归零 → 池够大了，再加没意义
- 还在涨 → 继续加（上限 32768）
</details>

<details>
<summary>Q2：你的 XDP 程序用 <code>XDP_TX</code> 把包原路反弹，但发送失败或数据不对。排除程序逻辑后，该查什么？</summary>

<b>答：</b>查 page_pool 创建时的 <code>dma_dir</code>。

v6.6 的校验和注释：

```c
/* DMA direction is either DMA_FROM_DEVICE or DMA_BIDIRECTIONAL.
 * DMA_BIDIRECTIONAL is for allowing page used for DMA sending,
 * which is the XDP_TX use-case.
 */
if (pool->p.flags & PP_FLAG_DMA_MAP) {
	if ((pool->p.dma_dir != DMA_FROM_DEVICE) &&
	    (pool->p.dma_dir != DMA_BIDIRECTIONAL))
		...  /* 创建失败 */
}
```

`PP_FLAG_DMA_MAP` 下，**只接受两个方向**：
- `DMA_FROM_DEVICE`：只能收，页面映射为"设备写、CPU 读"
- `DMA_BIDIRECTIONAL`：**XDP_TX 必须用这个**——同一块页面既要设备写（收）又要设备读（发）

如果你的 pool 是 `DMA_FROM_DEVICE`，反弹发送时映射方向不对，设备读不到正确数据。

这只对**你自己创建 page_pool** 的场景适用（写驱动、或用某些 XDP 框架）。
用现成驱动时，驱动已经按自己是否支持 XDP_TX 配好了——但确认一下 `dma_dir` 总没错。
</details>

<details>
<summary>Q3：你在生产环境跑了 tcpdump 抓包排障，抓完发现延迟指标比平时差，而且持续了一段时间才恢复。可能的原因？</summary>

<b>答：</b>两个叠加的原因，都和 refcnt 有关：

<b>① tcpdump 会 clone skb</b>。AF_PACKET 要给每个匹配的包复制一份给抓包 socket，
`skb_clone()` 共享数据区 → page 的 refcnt 变 2 → 按 page_pool 的规则，
释放时走 **`rx_pp_recycle_released_ref`**（refcnt > 1 就真释放回伙伴系统）。
于是<b>你抓的每个包，都让一个页面永久离开 page_pool</b>。

<b>② 池被抽干后需要时间恢复</b>。抓包期间池被持续抽空，之后要重新从伙伴系统
批量补充（`PP_ALLOC_CACHE_REFILL = 64` 一次），期间 `rx_pp_alloc_empty` 偏高、
分配走慢路径——这就是你看到的"持续了一段时间才恢复"。

验证：
```bash
ethtool -S eth0 | grep rx_pp_recycle_released_ref
# 抓包前后对比，会看到明显增长
```

<b>实践建议</b>：
- 排障时尽量用 **XDP 采样**（在 XDP 层只取你关心的包，其余 `XDP_DROP`），
  而不是在 skb 层全量抓包
- 必须抓包就加严格 filter（`-i eth0 'udp port 12345'`），减少 clone 数量
- 抓包窗口尽量短，抓完确认指标恢复
- 记住：**抓包工具本身是观测扰动源**，这在 HFT 系统上尤其明显
</details>

---

→ 前一篇：[01 page_pool 机制本体](01-page-pool.md)
→ 本章完，下一章：[chapter-05 XDP 架构](../../chapter-05-xdp-architecture/)
→ 相关：[chapter-03/02 驱动契约](../../chapter-03-tx-path-skbbuff/notes/02-txrx.md) · [chapter-06 AF_XDP](../../chapter-06-af-xdp/)
