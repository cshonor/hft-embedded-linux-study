## ③ 区 · Zones

并非所有 **物理页** 对内核都 **同等可访问** — **区（zone）** 按 **地址能力** 与 **用途** 划分 **伙伴系统 free list**。

#### 完整的区列表（v6.6 `mmzone.h` 实证）

区是按**条件编译**拼出来的枚举，不是固定四个：

```c
enum zone_type {
#ifdef CONFIG_ZONE_DMA
	ZONE_DMA,          /* 老 ISA DMA（<16MB） */
#endif
#ifdef CONFIG_ZONE_DMA32
	ZONE_DMA32,        /* 32 位 DMA 掩码设备（<4GB） */
#endif
	ZONE_NORMAL,
#ifdef CONFIG_HIGHMEM
	ZONE_HIGHMEM,
#endif
	ZONE_MOVABLE,      /* 只放可移动页，便于热拔与反碎片 */
#ifdef CONFIG_ZONE_DEVICE
	ZONE_DEVICE,       /* 设备内存（持久内存 pmem、GPU 显存等），不是普通 RAM */
#endif
	__MAX_NR_ZONES
};
```

| 区 | 用途 | 64 位服务器上是否常见 |
|----|------|----------------------|
| **ZONE_DMA** | **ISA 时代** DMA 只能寻址 **低 16MB** 物理 | 少数老设备驱动需要 |
| **ZONE_DMA32** | **32 位 DMA 掩码** 设备 — 可访问 **< 4GB** 物理 | ✅ 常见 |
| **ZONE_NORMAL** | 可 **永久映射** 进内核 **线性地址空间** 的页 | ✅ 主力 |
| **ZONE_HIGHMEM** | 无永久内核线性映射（**x86 32 位** 896MB 以上） | ❌ 64 位无 |
| **ZONE_MOVABLE** | 只放可移动页；**热拔 + 反碎片** | 按需开 |
| **ZONE_DEVICE** | **设备私有内存**（pmem / DAX / GPU），**不是普通 RAM** | 有持久内存时 |

> **LKD 时代只有前四个。** `ZONE_MOVABLE` 与 `ZONE_DEVICE` 是两个现代扩展，
> 作用完全不同：MOVABLE 是**逻辑策略**（让这片的页都可迁移，便于腾出连续空间），
> DEVICE 是**物理介质**（页根本不在系统 RAM 里，由设备驱动管理生命周期）。

#### 层级：node → zone → free_area

```
NUMA 节点（struct pglist_data，mmzone.h:1261）
   │  node_zones[MAX_NR_ZONES]
   ▼
zone（struct zone）  ── 水位线 _watermark[NR_WMARK] / watermark_boost
   │  free_area[MAX_ORDER]
   ▼
free_area（每个 order 一个）
   │  free_list[MIGRATE_TYPES]     ← 12.2 讲的迁移类型
   ▼
伙伴链表
```

> **`ZONE_NORMAL` 不够时会怎样**：跨 node / 跨 zone 的分配顺序由 **zonelist** 决定
> （`node_zonelists`，按 node 距离排序 + zone 优先级），而不是随便挑。
> 这就是 NUMA 机器上"本节点内存不够 → 去远端节点借页 → 延迟变高"的机制来源。

```
物理地址升高 ──────────────────────────────────────►

  ZONE_DMA   │  ZONE_DMA32  │     ZONE_NORMAL      │  ZONE_HIGHMEM
  (低地址)   │              │  (direct map 可达)    │  (需 kmap 临时)
             │              │                      │
         老 DMA 设备      现代 PCI DMA          内核 kmalloc 默认来源
```

#### 为何需要分区

| 约束 | 后果 |
|------|------|
| **DMA 寻址宽度** | 设备只能看 **低物理地址** — 须从 **ZONE_DMA*** 留页 |
| **32 位内核 VA 有限** | 内核 **直接映射窗口** 固定 — 超出部分 = **HIGHMEM** |
#### 水位线：每 zone 三档（v6.6 实证）

```c
/* include/linux/mmzone.h:653 */
enum zone_watermarks {
	WMARK_MIN,
	WMARK_LOW,
	WMARK_HIGH,
	NR_WMARK
};
/* 取值要加 watermark_boost： */
#define min_wmark_pages(z)  (z->_watermark[WMARK_MIN]  + z->watermark_boost)
#define low_wmark_pages(z)  (z->_watermark[WMARK_LOW]  + z->watermark_boost)
#define high_wmark_pages(z) (z->_watermark[WMARK_HIGH] + z->watermark_boost)
```

| 档位 | 触发什么 |
|------|---------|
| **低于 LOW** | 唤醒 **kswapd** 后台异步回收 |
| **低于 MIN** | 分配进入**同步直接回收**（`__GFP_DIRECT_RECLAIM` 时）；或**失败**（`GFP_ATOMIC` 等不允许回收的标志） |
| **高于 HIGH** | kswapd 认为够了，**停止回收** |

> **`watermark_boost` 是一个可叠加的临时抬升**（碎片事件后短时抬高水位，迫使回收更激进以制造连续块）。
> 它解释了"为什么 `cat /proc/zoneinfo` 里看到的 min 与理论值对不上"。

#### 64 位与嵌入式

| 平台 | HIGHMEM |
|------|---------|
| **x86-64 / arm64 服务器** | 通常 **无 ZONE_HIGHMEM** — 全物理可 direct map |
| **ARM32 3:1 分割** | 可能有 **HIGHMEM** — 驱动访问高端页用 **`kmap_local_page()`** |
| **SoC 小 RAM** | 区仍分 **DMA32/NORMAL** — **CMA** 常占 **NORMAL** 一段 |

> ⚠️ **版本更新：别再用 `kmap_atomic()`。**
> v6.6 `include/linux/highmem.h:135` 的注释已经把它标为 **Deprecated**：
> *"In fact a wrapper around kmap_local_page() which also disables pagefaults.
> Do not use in new code. Use kmap_local_page() instead."*
> 新代码一律用 **`kmap_local_page()` / `kunmap_local()`**（配对、可嵌套、不需要禁抢占）。
> 详见 [12.9](./section-12.9-高端内存的映射.md)。

#### `gfp_zone()` 与分配回落

| 行为 | 说明 |
|------|------|
| **`GFP_KERNEL`** | 优先 **NORMAL**；可 **回落** 到其他 zone（视标志） |
| **`GFP_DMA` / `GFP_DMA32`** | **强制** 从对应 zone 取 — 失败则 NULL |
| **`__GFP_HIGHMEM`** | 允许分配 **不可直接映射** 的页 — 访问前 **kmap** |

**HFT：** 用户态 **DMA-BUF / RDMA** 注册内存时，驱动在内核 **`dma_alloc_coherent`** 从 **合适 zone** 拿 **物理连续 + 设备可见** 的页 — 懂 zone 才懂 **`swiotlb` bounce buffer**（物理页不在设备掩码内时 **拷贝**）。

> **HFT 补充：zone 与 NUMA 的交集才是尾延迟杀手。**
> 分配时先走**本地 node 的 zonelist**，本地不够才会去**远端 node 借页**
> ——跨 socket 访问（Intel UPI / AMD Infinity Fabric）单次多出几十到上百纳秒。
> 表现是"内存还很充足，但延迟周期性变差"。对策：
> ① 绑核 + `numactl --membind`（或 `mbind(MPOL_BIND)`）把数据与线程锁在同 node；
> ② 关键数据结构**预分配**并 `mlock`，避免运行中跨 node 分配；
> ③ 观察 `/proc/zoneinfo` 的 free 与 `numastat` 的 `other_node` 指标——
> `other_node` 持续上涨就是跨节点分配在发生。

→ [06 Gorman Ch2 内存区域](../../../06-linux-mm/chapter-02-describing-physical-memory/notes/section-2-内存区域.md) · [Ch 12.9 HIGHMEM](./section-12.9-高端内存的映射.md)



<details>
<summary>自测题（点击展开）</summary>

**Q1.** ZONE_DMA、ZONE_NORMAL、ZONE_HIGHMEM 分别是什么？x86_64 还有 HIGHMEM 吗？

<details><summary>答案</summary>

ZONE_DMA：< 16MB（ISA DMA 限制）；ZONE_NORMAL：16MB-896MB（直接映射）；ZONE_HIGHMEM：> 896MB（需 kmap 临时映射）。x86_64 没有 HIGHMEM——64 位地址空间足够直接映射所有物理内存。HIGHMEM 是 32 位内核的历史包袱。

</details>

**Q2.** 为什么 GFP_KERNEL 可能从 ZONE_NORMAL 分配而 GFP_DMA 从 ZONE_DMA？

<details><summary>答案</summary>

GFP_DMA 保证物理地址 < 16MB（ISA 设备只能寻址 24 位地址）。GFP_KERNEL 无地址限制，优先从 ZONE_NORMAL 分配（直接映射，速度快）。如果 ZONE_NORMAL 不足，buddy 系统会从 ZONE_DMA 挪页（fallback），但会保留 DMA 备用页防止饿死。

</details>

**Q3.** v6.6 的 zone 枚举里，LKD 没提到的两个区是什么？各自解决什么问题？

<details><summary>答案</summary>

`ZONE_MOVABLE` 和 `ZONE_DEVICE`。`ZONE_MOVABLE` 是**逻辑策略**：只放可移动页，让这片区域始终保持可迁移，用于内存热拔和制造连续物理空间（配合 12.2 的 migratetype，注意 `alloc_contig_range()` 迁移也可能失败——如果页被长期 pin 住）。`ZONE_DEVICE` 是**物理介质**：页根本不在系统 RAM 里，而是设备私有内存（持久内存 pmem/DAX、GPU 显存），生命周期由设备驱动管理而非伙伴系统。两者性质完全不同，一个管策略、一个管介质。

</details>

**Q4.** zone 的三档水位线各自触发什么？为什么 `/proc/zoneinfo` 里看到的 min 与计算值对不上？

<details><summary>答案</summary>

v6.6 `mmzone.h:653` 定义 `WMARK_MIN / WMARK_LOW / WMARK_HIGH`：低于 **LOW** 唤醒 **kswapd** 异步回收；低于 **MIN** 进入同步直接回收（允许回收的 gfp 标志）或直接失败（`GFP_ATOMIC` 等不允许回收时）；高于 **HIGH** kswapd 停止。对不上的原因是取值宏都加了 `watermark_boost`：`min_wmark_pages(z) = z->_watermark[WMARK_MIN] + z->watermark_boost`。`watermark_boost` 是碎片事件后的**临时抬升**，用来迫使回收更激进以制造连续块，所以是动态值。

</details>

**Q5.** 为什么新代码不该再使用 `kmap_atomic()`？

<details><summary>答案</summary>

v6.6 `include/linux/highmem.h:135` 已把它标记为 **Deprecated**，注释原文说它"实际上只是 `kmap_local_page()` 的包装，额外关掉 pagefault"，并明确要求"Do not use in new code. Use kmap_local_page() instead."。新代码用 `kmap_local_page()` / `kunmap_local()`：配对使用、可以嵌套（严格后进先出）、且**不需要禁用抢占**，语义更清晰、开销更低。

</details>

</details>
---
