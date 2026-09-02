# Ch 11 §3 分配交换槽 (Allocating Swap Slots)

> **Understanding the Linux Virtual Memory Manager** · Mel Gorman · **跳过 ⚪**
> 源码核验：Linux **v6.6**（`mm/swapfile.c` / `include/linux/swap.h`）

---

## 本节讲什么

本节回答：**换出时，内核怎么在 swap 区里挑一个空闲 slot？**

原书的核心思想是 **cluster（簇）**——尽量连续分配多个 slot，把随机 seek 变顺序 I/O。v6.6 里这个思想还在，但**按介质分裂成两条路径**：HDD 走"顺序簇"，SSD 走"散开 + per-CPU"。本节沿 `scan_swap_map_slots` 源码走一遍。

---

## 1. 簇分配总览：`scan_swap_map_slots`（`mm/swapfile.c:799`）

```c
#define SWAPFILE_CLUSTER	256        /* :277 一个簇 = 256 页 = 1MB（4K 页时） */
#define SWAP_BATCH          64         /* swap.h:35 一次最多批量分配 64 个 */
#define LATENCY_LIMIT       256        /* :285 扫描每 256 次 cond_resched */
```

`scan_swap_map_slots` 的核心注释（`:811-820`）浓缩了整个演进史：

> 我们尝试**顺序分配**，一旦分配满 `SWAPFILE_CLUSTER` 页就退回 first-free，开新簇——防止 swap 页散满整个分区，减少 seek。
> 但现在我们会**先找空簇**。而且（SSD 上）让 swap 页散开——Hugh。

```
scan_swap_map_slots(si, usage, nr, slots[])
    │
    ├─ SSD 路径（si->flags & SWP_SOLIDSTATE + cluster_info）
    │     scan_swap_map_try_ssd_cluster()   ← 从 per-CPU 簇取
    │
    ├─ HDD 路径（cluster_nr 倒计时）
    │     cluster_nr 用尽 → 从 lowest_bit 开始找空簇
    │     顺序填满一个 256 页的簇
    │
    └─ 逐 slot 分配
          swap_map[offset] = SWAP_HAS_CACHE   ← 标记"有 cache"
          slots[n_ret++] = swp_entry(type, offset)
```

---

## 2. HDD 路径：顺序簇，减少 seek

```c
if (unlikely(!si->cluster_nr--)) {                 /* :838 簇倒计时用尽 */
    if (si->pages - si->inuse_pages < SWAPFILE_CLUSTER) {  /* :839 空间不足 */
        si->cluster_nr = SWAPFILE_CLUSTER - 1;
        goto checks;
    }
    /* 从分区头开始，找一个 256 页的空簇 */
    scan_base = offset = si->lowest_bit;           /* :852 */
    last_in_cluster = offset + SWAPFILE_CLUSTER - 1;
    for (; last_in_cluster <= si->highest_bit; offset++) {
        if (si->swap_map[offset])
            last_in_cluster = offset + SWAPFILE_CLUSTER;  /* 跳过占用 */
        else if (offset == last_in_cluster) {      /* 找到连续 256 空页 */
            si->cluster_next = offset;             /* :862 记录簇起点 */
            si->cluster_nr = SWAPFILE_CLUSTER - 1; /* :863 重置倒计时 */
            goto checks;
        }
        if (unlikely(--latency_ration < 0)) {      /* :866 长扫描让出 CPU */
            cond_resched();
            latency_ration = LATENCY_LIMIT;
        }
    }
}
```

| 要点 | 说明 |
|------|------|
| **簇 = 256 页 = 1MB**（4K 页时） | 一次连续写 1MB，把随机 seek 变顺序 I/O |
| **`cluster_nr` 倒计时** | 每分配一页减一，减到 0 就换新簇 |
| **"先找空簇"** | 不是逐页散找，而是**一次性圈定一个 256 页的空洞**，再顺序填满 |
| **`LATENCY_LIMIT`** | 大分区扫描可能很长，每 256 次 `cond_resched` 让出 CPU（`:866`），避免长时间占用 |

**直觉**：HDD 的 seek 极贵，所以要让"同时换出的页"落在**相邻磁盘块**上，换入时就能顺序读。簇就是为此设计的"批量化 + 局部化"。

---

## 3. SSD 路径：散开 + per-CPU，减锁竞争

```c
if (si->flags & SWP_SOLIDSTATE)                   /* :828 SSD */
    scan_base = this_cpu_read(*si->cluster_next_cpu);  /* per-CPU 起点 */
else
    scan_base = si->cluster_next;                 /* HDD 全局起点 */
```

SSD seek 廉价，问题从"减少 seek"变成"**减少锁竞争**"（注释 `:824-826`）：

```c
/* swap.h:250  SSD 专用的簇跟踪结构 */
struct swap_cluster_info {
    spinlock_t lock;              /* 保护本簇 + 对应 swap_map 元素 */
    unsigned int data:24;         /* 空闲时=下一簇号；否则=簇使用计数 */
    unsigned int flags:8;         /* CLUSTER_FLAG_FREE / NEXT_NULL / HUGE */
};
#define CLUSTER_FLAG_FREE 1       /* :260 此簇空闲 */

/* swap.h:269  每 CPU 一个分配位置 */
struct percpu_cluster {
    struct swap_cluster_info index;  /* 当前簇 */
    unsigned int next;               /* 簇内下一个分配 offset */
};
```

| 结构 | 作用 |
|------|------|
| **`swap_cluster_info`** | SSD 上每个簇一个，带**自己的 spinlock**——不同 CPU 分配不同簇时**不争同一把锁** |
| **`percpu_cluster`** | 每 CPU 记"自己在哪个簇、簇内到哪了"，让 swap out 尽量**各 CPU 走各的簇** |
| **`free_clusters`** | 空闲簇链表，取簇 O(1) |

**直觉**：SSD 上"散开"无害（无 seek 惩罚），但"多个 CPU 同时换出争同一把 swap 锁"有害。于是**牺牲顺序性，换取 per-CPU 无锁（少锁）分配**——和 §2 的 `page_alloc` per-CPU 冷热页表、§1 的 percpu 思想一脉相承。

---

## 4. `swap_map[]`：slot 的 usage count

分配 slot 时更新 `swap_map[offset]`：

```c
WRITE_ONCE(si->swap_map[offset], usage);   /* :916 标记 usage = SWAP_HAS_CACHE */
```

`swap_map[]` 是**每 slot 一个字节的引用计数**（§1），换出路径传入的 `usage` 是 `SWAP_HAS_CACHE`（`swap.h:229`，0x40）——表示"这个 slot 有 cache 页正在写盘"。之后随着更多 PTE 指向它、或 cache 释放，计数会增减。

特殊值（`swap.h:233-235`）：

| 值 | 含义 |
|----|------|
| `SWAP_MAP_MAX` (0x3e) | 引用计数上限 |
| `SWAP_MAP_BAD` (0x3f) | **坏块**（跳过不分配） |
| `SWAP_MAP_SHMEM` (0xbf) | shmem/tmpfs 独占 |

---

## 5. 全局调度：`get_swap_pages`（`swapfile.c:1047`）

`scan_swap_map_slots` 是**单个 swap 区内**找 slot；`get_swap_pages` 是**跨多个 swap 区**挑一个：

```c
int get_swap_pages(int n_goal, swp_entry_t swp_entries[], int entry_size)
{
    ...
    n_goal = min3((long)n_goal, (long)SWAP_BATCH, avail_pgs);  /* :1066 最多 64 */

    node = numa_node_id();                                    /* :1071 本节点 */
    plist_for_each_entry_safe(si, next, &swap_avail_heads[node], avail_lists[node]) {
        plist_requeue(&si->avail_lists[node], &swap_avail_heads[node]);  /* :1074 轮转 */
        ...
        n_ret = scan_swap_map_slots(si, SWAP_HAS_CACHE, n_goal, swp_entries); /* :1097 */
        if (n_ret)
            goto check_out;
        ...
    }
}
```

| 要点 | 说明 |
|------|------|
| **`swap_avail_heads[node]`** | **每个 NUMA 节点一条**可用 swap 链表（`swap.h:324` `avail_lists[]`），就近选择 |
| **`prio` 排序** | 链表按优先级排序，**高优先级先用**（§1 的 `prio` 落点） |
| **`plist_requeue`** | 同优先级之间**轮转**（round-robin），避免总压在一个区上 |
| **`SWAP_BATCH = 64`** | 一次最多批量分配 64 个 slot，减少锁往返 |

---

## 6. HFT / 嵌入式关联

| 场景 | 关联 |
|------|------|
| **HDD vs SSD 分叉** | 同一份代码按介质特性分两条路——HFT 里"存储介质决定算法"是常识（列存 vs 行存、顺序 vs 随机） |
| **per-CPU 减锁** | SSD 用 per-CPU 簇把全局锁打散，是内核"**用冗余换锁竞争下降**"的又一例——HFT 的 per-thread 队列同构 |
| **`cond_resched` 与 LATENCY_LIMIT** | 长扫描主动让出 CPU，控制延迟尖刺——低延迟系统的基本素养 |
| **结论** | 即便有 cluster 优化，swap 仍是**磁盘级延迟**，HFT 首选 `mlock` 彻底避免 |

---

## 7. 衔接

- 下节 [§4 交换缓存](./section-4-交换缓存-—-核心.md)：slot 分配后，页怎么进 swap cache 完成写盘
- `swap_map` 字段来源：[§1 描述交换区](./section-1-描述交换区.md)

---

<details>
<summary>代码自测（Q&A）</summary>

**Q1：`SWAPFILE_CLUSTER` 是多少？为什么取这个值？**
A：默认 **256 页 = 1MB**（4K 页时；THP 开启时 = `HPAGE_PMD_NR` = 512）。取 1MB 是"顺序 I/O 收益 vs 簇碎片"的折中——一次连续写 1MB 能显著摊薄 seek 成本，又不会因为簇太大导致分配粒度太粗、浪费空间。

**Q2：HDD 和 SSD 的 slot 分配策略有什么本质区别？为什么？**
A：HDD 走**顺序簇**——圈定 256 页空簇后顺序填满，把随机 seek 变顺序 I/O；SSD 走**散开 + per-CPU**——seek 廉价，改用 per-CPU 簇减少锁竞争。本质是"**优化目标随介质特性转移**"：HDD 优化 seek，SSD 优化锁竞争。

**Q3：`struct swap_cluster_info` 的 `data` 字段为什么是 union 语义（空闲=下一簇号 / 占用=使用计数）？**
A：和 vmalloc 的 `vmap_area` union 同思路——一个簇**要么空闲要么占用**。空闲时 `data` 存"下一个空闲簇的编号"（构成链表），占用时存"这个簇还有多少空 slot"。两个语义互斥，复用 24 bit 存储。

**Q4：`get_swap_pages` 里的 `plist_requeue` 是干什么的？**
A：它把刚用过的 swap 区**移到同优先级链表的末尾**，实现同优先级多区之间的**轮转（round-robin）**。避免所有换出都压到第一个区上，让同优先级的几个区负载均衡。更高优先级的区仍优先被选。

**Q5：`scan_swap_map_slots` 里的 `LATENCY_LIMIT` 解决什么问题？**
A：在 HDD 路径找空簇时，可能要扫很长一段 `swap_map[]`（大分区几百万项）。若一直扫不让出 CPU，会长时间占用调度。`LATENCY_LIMIT=256` 让扫描每 256 次就 `cond_resched()` 一次，控制延迟尖刺——这是内核里"长循环主动让权"的标准写法。

</details>
