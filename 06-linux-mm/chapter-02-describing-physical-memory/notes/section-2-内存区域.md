# Ch 2 §2 内存区域 (Zones)

> **Understanding the Linux Virtual Memory Manager** · Mel Gorman · **精读 🔴**
> 源码核验：Linux **v6.6**（`include/linux/mmzone.h`）

---

## 本节讲什么

节点太粗——一个 NUMA 节点里可能混着「只有 ISA 设备能 DMA 的低端内存」和「普通内存」。内核把节点再切成 **Zone**，本节回答：

1. 为什么分 zone？现代内核有哪些 zone 类型？
2. `struct zone` 的字段级真身——尤其是**三个 page 计数**和水位数组？
3. 水位怎么触发回收（这是 HFT 抖动的地基）？

原书 2.4/2.6 语境讲 `ZONE_DMA / ZONE_NORMAL / ZONE_HIGHMEM` 三件套；v6.6 的 `enum zone_type`（`mmzone.h:715`）已扩到六个，且**全部由 CONFIG 开关决定是否编译进枚举**。

---

## 1. 为什么分 zone + v6.6 的 zone 类型

根本原因：**不是所有物理内存对所有用途都平等**。有的设备 DMA 只能寻址低 16MiB，有的 32 位内核线性映射窗口只有 ~896MiB。

```c
enum zone_type {            /* mmzone.h:715 */
#ifdef CONFIG_ZONE_DMA
    ZONE_DMA,               /* 低 16MiB：老式 ISA DMA 设备专用 */
#endif
#ifdef CONFIG_ZONE_DMA32
    ZONE_DMA32,             /* 32 位 DMA 可寻址（x86_64 上通常 0~4GiB） */
#endif
    ZONE_NORMAL,            /* 内核线性映射可直接覆盖的「普通」内存 */
#ifdef CONFIG_HIGHMEM
    ZONE_HIGHMEM,           /* 32 位内核线性映射窗口之外的内存（§4） */
#endif
    ZONE_MOVABLE,           /* 可迁移页：内存热拔/大页/离线更易成功 */
#ifdef CONFIG_ZONE_DEVICE
    ZONE_DEVICE,            /* 非易失内存/设备内存（pmem、HBM、GPU） */
#endif
    __MAX_NR_ZONES
};
```

| Zone | 典型用途 | x86_64 上是否存在 |
|------|----------|-------------------|
| `ZONE_DMA` | ISA DMA 设备 | 通常**没有**（现代 x86_64 用 DMA32 覆盖） |
| `ZONE_DMA32` | 只能 32 位 DMA 的设备 | **有**（0~4GiB） |
| `ZONE_NORMAL` | 通用 | **有**（主体） |
| `ZONE_HIGHMEM` | 32 位内核补丁 | **没有**（`CONFIG_HIGHMEM` 未定义） |
| `ZONE_MOVABLE` | 可迁移页（内存热拔/大页） | 视 `kernelcore/movablecore` 启动参数 |
| `ZONE_DEVICE` | pmem/HBM/GPU | 视 `CONFIG_ZONE_DEVICE` |

**关键：zone 类型是「按能力分层」，不是「按用途分类」。** `ZONE_DMA32` 和 `ZONE_NORMAL` 可能物理上重叠——一块 2GiB 的内存，低 4GiB 部分同时属于 `ZONE_DMA32` 和 `ZONE_NORMAL` 候选，分配时按 `gfp_mask` 里的 `GFP_DMA32` 等标志决定走哪个 zone。

---

## 2. `struct zone` 真身（v6.6 `mmzone.h:810`）

```c
struct zone {
    /* 只读字段 */
    unsigned long _watermark[NR_WMARK];   /* 4 个水位：MIN/LOW/HIGH/PROMO */
    unsigned long watermark_boost;        /* 临时抬高的水位（内存碎片化防御） */
    unsigned long nr_reserved_highatomic; /* 保留给原子分配的高位内存 */
    long lowmem_reserve[MAX_NR_ZONES];    /* 低端 zone 的应急保留（防 OOM） */

    int node;                             /* 所属节点 id（CONFIG_NUMA） */
    struct pglist_data *zone_pgdat;       /* 反指所属节点 */
    struct per_cpu_pages __percpu *per_cpu_pageset;  /* 每 CPU 页缓存（§5） */

    unsigned long zone_start_pfn;         /* zone 起始页框号 */

    atomic_long_t managed_pages;          /* buddy 真正管理的页 */
    unsigned long spanned_pages;          /* 跨度（含空洞） */
    unsigned long present_pages;          /* 实际存在（不含空洞） */

    const char *name;                     /* "Normal"/"DMA32"/... */

    /* 写密集字段（页分配器用） */
    CACHELINE_PADDING(_pad1_);
    struct free_area free_area[MAX_ORDER + 1];  /* buddy 空闲链表（Ch 6） */
    unsigned long flags;
    spinlock_t lock;                      /* 保护 free_area 的自旋锁 */

    CACHELINE_PADDING(_pad2_);
    /* 压缩（compaction）与 vmstat 字段 ... */
    CACHELINE_PADDING(_pad3_);
    atomic_long_t vm_stat[NR_VM_ZONE_STAT_ITEMS];  /* zone 统计计数 */
} ____cacheline_internodealigned_in_smp;
```

三个 page 计数是本节最容易被问倒的细节（源码注释讲得极清楚）：

| 计数 | 公式 | 含义 |
|------|------|------|
| `spanned_pages` | `zone_end_pfn - zone_start_pfn` | 地址跨度，**含空洞** |
| `present_pages` | `spanned - 空洞` | 实际存在的物理页 |
| `managed_pages` | `present - reserved` | **buddy 系统真正能分配的页**（扣掉 bootmem 保留等） |

**HFT 直觉：** `/proc/meminfo` 的 `MemFree` 之类，对应的是 `managed_pages` 里空闲的部分。`managed_pages` 才是页分配器和回收器算水位、算阈值的基数——所以看内存「够不够」，认准 managed 口径。

---

## 3. 水位线：`enum zone_watermarks`（v6.6 `mmzone.h:652`）

```c
enum zone_watermarks {
    WMARK_MIN,
    WMARK_LOW,
    WMARK_HIGH,
    WMARK_PROMO,     /* v6.6 新增：NUMA promotion 水位 */
    NR_WMARK
};
/* 取水位必须走宏，因为还要叠加 watermark_boost */
#define wmark_pages(z, i)  (z->_watermark[i] + z->watermark_boost)
#define low_wmark_pages(z) (z->_watermark[WMARK_LOW]  + z->watermark_boost)
```

```
空闲页数量
  HIGH ────────────────  正常分配
   LOW ────────────────  kswapd 被唤醒，后台异步回收
   MIN ────────────────  分配器【同步】回收（direct reclaim，延迟尖刺）
```

| 水位 | 触发 | 代价 |
|------|------|------|
| `WMARK_HIGH` | 空闲充足 | 无 |
| `WMARK_LOW` | 空闲跌破 | 唤醒 `kswapd`（`wakeup_kswapd()`，`mmzone.h:1424` 声明）后台回收 |
| `WMARK_MIN` | 空闲告急 | 分配路径**同步回收**——调用进程被拖慢 |
| `WMARK_PROMO` | NUMA 下 | 触发跨节点**页迁移（promotion）**，把远端页迁回本地 |

**HFT 关联（原笔记已有，这里点透机制）：** `direct reclaim` 慢在哪——分配路径里同步扫描 LRU、写回脏页、可能还要等 I/O，**一次 `malloc` 背后可能藏几毫秒到几十毫秒的停顿**。`mlock` 只防自己的页被换出，防不了**全局**跌破 `WMARK_MIN` 触发 direct reclaim。所以 HFT 要监控 `/proc/vmstat` 的 `allocstall_*` / `pgscan_*` 计数。

---

## 4. 等待队列表 (Wait Queue Table)

页做 I/O（换入/换出、块设备读写）时被锁（`PG_locked`），等它的进程要睡。**不为每个 page 单独建等待队列**（内存开销太大），而是在 **zone 级别**维护哈希桶式等待结构，多个 page 共享。回收/swap 路径会反复碰到：`PG_locked` → 在 zone 等待队列上 sleep → I/O 完成唤醒。

---

## 5. 代码示例：`/proc/zoneinfo`

```bash
$ cat /proc/zoneinfo | head -30
Node 0, zone      DMA
  per-node stats
      nr_inactive_anon 0
      nr_active_anon 0
  pages free     3975          # 空闲页
        min      10            # WMARK_MIN
        low      24            # WMARK_LOW
        high     38            # WMARK_HIGH
        spanned  4095          # 跨度
        present  3998          # 实际存在
        managed  3975          # buddy 管理
        protection: (0, 1024, 1024, 1024, ...)   # lowmem_reserve
```

三个计数 `spanned/present/managed` 在这里直接可见，水位 `min/low/high` 也一一对应枚举。

---

## 6. HFT / 嵌入式关联

| 现象 | 本节机制的兑现 |
|------|----------------|
| 内存「够」却偶发停顿 | 跌破 `WMARK_MIN` → direct reclaim 同步回收 |
| 绑 NUMA 仍可能回退 | 本地 zone 水位告急 → 回退远端 node（§1 zonelist） |
| 大页分配失败 | `ZONE_MOVABLE` 不足 + 碎片化 → compaction 失败（`compact_stall`） |
| 原子上下文分配不能睡 | `nr_reserved_highatomic` 为 `GFP_ATOMIC` 保留应急页 |

---

## 7. 衔接

- 上节 [§1 内存节点](./section-1-内存节点.md)：节点内部再分 zone
- 下节 [§3 物理页框](./section-3-物理页框.md)：`free_area[]` 里的 `struct page`
- [§4 高端内存](./section-4-高端内存.md)：`ZONE_HIGHMEM` 专讲
- 分配落地：[Ch 6 物理页分配](../../chapter-06-physical-page-allocation/)（`free_area` buddy 系统）

---

<details>
<summary>代码自测（Q&A）</summary>

**Q1：x86_64 上 `ZONE_DMA` 为什么通常不存在？**
A：`enum zone_type` 里 `ZONE_DMA` 被 `#ifdef CONFIG_ZONE_DMA` 包着，而 x86_64 不定义 `CONFIG_ZONE_DMA`（它用 `ZONE_DMA32` 覆盖 0~4GiB）。所以 64 位机器上 `ZONE_DMA` 这个名字干脆**不编译进枚举**，`/proc/zoneinfo` 里也看不到它。

**Q2：`present_pages` 和 `managed_pages` 差的那部分去哪了？**
A：被 `bootmem`/`memblock` 早期分配、内核镜像、`struct page` 数组本身、CMA 保留区等**预留**掉了。`managed = present - reserved`，buddy 只能从 managed 里往外发页。所以一台 32GiB 机器 `MemTotal` 常常略小于 32GiB。

**Q3：为什么取水位要走 `wmark_pages()` 宏而不是直接读 `_watermark[]`？**
A：还要叠加 `watermark_boost`。内存在碎片化时内核会临时「抬高」水位（boost），逼回收器更早介入、留出更多连续空闲页。直接读数组会拿到未加 boost 的裸值，和真实判断不一致。

**Q4：`WMARK_PROMO` 是干什么的？和前三者有什么本质不同？**
A：前三者（MIN/LOW/HIGH）管的是**回收**（内存不够就踢页），`PROMO` 管的是**NUMA 迁移**（内存够但页在远端，把它迁回本地）。它解决的是「本地慢/远端慢」的**延迟**问题，不是「内存不足」的**容量**问题。

**Q5：`struct zone` 里为什么有这么多 `CACHELINE_PADDING`？**
A：`free_area[]`、`lock` 这些是**写密集**字段，`_watermark[]` 等是**读密集**字段。把它们用 padding 隔到不同 cache line，避免读锁的 CPU 和写 free_area 的 CPU 互相打爆对方的 cache line（false sharing）。这与 §5 讲的 2.6 引入 padding 是同一件事，v6.6 用 `CACHELINE_PADDING(_pad1_/2_/3_)` 宏固化了下来。

</details>

---
