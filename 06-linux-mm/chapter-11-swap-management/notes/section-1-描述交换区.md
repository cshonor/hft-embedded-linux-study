# Ch 11 §1 描述交换区 (`swap_info_struct`)

> **Understanding the Linux Virtual Memory Manager** · Mel Gorman · **跳过 ⚪**
> 源码核验：Linux **v6.6**（`include/linux/swap.h` / `mm/swapfile.c`）

---

## 本节讲什么

本节回答：**内核用什么结构描述一个已激活的 swap 区（分区或文件）？**

原书答案是 `swap_info_struct` + 静态数组 `swap_info[MAX_SWAPFILES]`，这个骨架 v6.6 没变。但**细节大幅演进**：`MAX_SWAPFILES` 不再是"恰好 32"而是"编码位宽约束下的上限"，`swap_info_struct` 里塞进了 SSD 专用 cluster 结构、per-CPU 分配位置、swap extent 红黑树。本节落到 v6.6 字段级。

---

## 1. 全局布局：`swap_info[]` 数组

```c
/* mm/swapfile.c */
struct swap_info_struct *swap_info[MAX_SWAPFILES];   /* 全局静态数组 */
```

- 每个**已激活** swap 区（分区或文件）占一个 `swap_info_struct`，下标就是它的 **`type`**（`swap_info_struct.type` 字段）。
- `type` 这个"奇怪的名字"（源码注释 `:287` 明说 "strange name for an index"）就是**数组下标**——它被编码进 `swp_entry_t`（§2），所以必须全局唯一且有限。

### `MAX_SWAPFILES`：从"恰好 32"到"编码约束"

```c
/* include/linux/swap.h */
#define MAX_SWAPFILES_SHIFT	5                       /* :50  type 只占 5 bit */
#define MAX_SWAPFILES \
	((1 << MAX_SWAPFILES_SHIFT) - SWP_DEVICE_NUM - \
	 SWP_MIGRATION_NUM - SWP_HWPOISON_NUM)           /* :117 减去保留类型 */
```

| 概念 | 说明 |
|------|------|
| **`MAX_SWAPFILES_SHIFT = 5`** | `type` 字段**最多 5 bit** → 2⁵ = 32 种 type |
| **保留 type** | 32 里要**扣掉**几类特殊编码：`SWP_DEVICE_*`（device-private 内存，4 个）、`SWP_MIGRATION_*`（页迁移，2 个）、`SWP_HWPOISON`（硬件坏页，1 个） |
| **实际可用** | 32 − 7 = **约 25 个真实 swap 区**上限（不是原书说的"恰好 32 个 swap 区"） |

**关键直觉**：`MAX_SWAPFILES` 的**真正约束是 `swp_entry_t` 里 `type` 的位宽**（5 bit），不是数组大小。因为 `type` 要被打包进 PTE 的 swap entry（§2），位宽一旦定了，swap 区数量上限也就定了。

---

## 2. `struct swap_info_struct`（`swap.h:282`）

```c
struct swap_info_struct {
    struct percpu_ref users;        /* :283 引用计数，防 swap 设备被提前释放 */
    unsigned long     flags;        /* :284 SWP_* 标志（见 §1.3） */
    signed short      prio;         /* :285 swap 优先级（多区时的使用顺序） */
    struct plist_node list;         /* :286 swap_active_head 链表节点 */
    signed char       type;         /* :287 数组下标（strange name for an index） */
    unsigned int      max;          /* :288 swap_map 的 extent（slot 总数） */
    unsigned char    *swap_map;     /* :289 每 slot 一个 usage count（vmalloc 分配） */
    struct swap_cluster_info *cluster_info;   /* :290 簇信息（仅 SSD） */
    struct swap_cluster_list free_clusters;   /* :291 空闲簇链表 */
    unsigned int      lowest_bit;   /* :292 swap_map 里第一个空闲 slot */
    unsigned int      highest_bit;  /* :293 最后一个空闲 slot */
    unsigned int      pages;        /* :294 可用页总数 */
    unsigned int      inuse_pages;  /* :295 当前已用页数 */
    unsigned int      cluster_next; /* :296 下次分配的可能位置（HDD 顺序） */
    unsigned int      cluster_nr;   /* :297 到下次换簇的倒计时 */
    struct percpu_cluster __percpu *percpu_cluster;  /* :299 每 CPU 分配位置（SSD） */
    struct rb_root    swap_extent_root;  /* :300 swap extent 红黑树（§7） */
    struct block_device *bdev;      /* :301 块设备或 swap 文件所在 bdev */
    struct file      *swap_file;    /* :302 swap 文件句柄 */
    spinlock_t         lock;        /* :305 保护 swap_map/扫描相关字段 */
    ...
    struct plist_node avail_lists[]; /* :324 每个 NUMA 节点一个 avail 链表 */
};
```

### 字段速查（分类）

| 分类 | 字段 | 作用 |
|------|------|------|
| **身份** | `type` / `prio` | 数组下标 + 优先级（决定多区使用顺序） |
| **slot 状态** | `swap_map[]` / `max` / `lowest_bit` / `highest_bit` | **每 slot 一个 usage count**，快速定位空闲区间 |
| **统计** | `pages` / `inuse_pages` | 总页数 / 已用页数（`/proc/swaps` 的 Size/Used 来源） |
| **分配加速** | `cluster_next` / `cluster_nr`（HDD）/ `percpu_cluster`（SSD） | 簇分配的位置缓存（§3） |
| **映射** | `swap_extent_root` | 文件 swap 的"逻辑 slot ↔ 磁盘块"红黑树（§7） |
| **设备** | `bdev` / `swap_file` / `flags` | 底层块设备/文件 + `SWP_*` 属性 |

---

## 3. `SWP_*` 标志（`swap.h:207-223`）

| 标志 | 值 | 含义 |
|------|----|------|
| `SWP_USED` | `1<<0` | 该 `swap_info[]` 槽位已被占用 |
| `SWP_WRITEOK` | `1<<1` | **允许写入**（换出前必须置位） |
| `SWP_DISCARDABLE` | `1<<2` | 块设备支持 discard（TRIM） |
| `SWP_SOLIDSTATE` | `1<<4` | **SSD**——seek 廉价，分配策略走 SSD 路径 |
| `SWP_BLKDEV` | `1<<6` | 是**块设备**（而非文件） |
| `SWP_ACTIVATED` | `1<<7` | `swap_activate` 成功后置位 |
| `SWP_FS_OPS` | `1<<8` | **走文件系统 I/O**（文件 swap） |
| `SWP_STABLE_WRITES` | `1<<11` | 不覆盖 `PG_writeback` 页（避免重写） |
| `SWP_SYNCHRONOUS_IO` | `1<<12` | **同步 I/O 更高效**（如 zram） |

`SWP_SOLIDSTATE` 和 `SWP_SYNCHRONOUS_IO` 是 v6.6 里最关键的现代标志：前者触发 SSD 的散开分配（§3），后者让 zram 这类"同步 I/O 反而更快"的设备跳过异步块层（§5）。

---

## 4. swap 区统计：`/proc/swaps`

```
$ cat /proc/swaps
Filename    Type      Size       Used     Priority
/dev/sda2   partition 2097148    0        -2
/swapfile   file      1048576    0        -3
```

- **Size/Used** ← `pages` / `inuse_pages`
- **Priority** ← `prio`（数字越大越先用；默认分区 -1，文件 -2）

---

## 5. HFT / 嵌入式关联

| 场景 | 关联 |
|------|------|
| **`prio` 分级** | 多 swap 区时把**快设备（NVMe/SSD）设高优先级**、慢设备（HDD）设低优先级——和 HFT 里"热数据放快速介质"同构 |
| **`SWP_SYNCHRONOUS_IO` + zram** | 嵌入式/低延迟场景常用 **zram（内存压缩块设备）当 swap**，它同步 I/O 比异步块层更快——理解这个 flag 才能明白 zram 的性能逻辑 |
| **`inuse_pages` 监控** | `swapoff` 前必须先看 `inuse_pages`，非零就意味要触发昂贵的 `try_to_unuse`（§6） |

---

## 6. 衔接

- 下节 [§2 映射 PTE 到交换项](./section-2-映射-PTE-到交换项.md)：`type` 怎么被编码进 PTE
- slot 状态表 `swap_map[]`：[§3 分配交换槽](./section-3-分配交换槽.md)

---

<details>
<summary>代码自测（Q&A）</summary>

**Q1：`swap_info_struct.type` 为什么源码注释说它是 "strange name for an index"？**
A：因为它根本不是"类型"，而是 `swap_info[]` 数组的**下标**。它必须被编码进 `swp_entry_t`（§2）的 5 bit `type` 字段，所以全局唯一且有限。命名有历史包袱——早期它确实表达"swap 类型"，后来简化成"数组索引"，名字却没改。

**Q2：`MAX_SWAPFILES` 真正的上限是谁决定的？为什么不是 32？**
A：是 `swp_entry_t` 里 `type` 字段的位宽决定的（`MAX_SWAPFILES_SHIFT = 5`，即 5 bit = 32 种）。但 32 里要扣掉 `SWP_DEVICE_*`（4）、`SWP_MIGRATION_*`（2）、`SWP_HWPOISON`（1）共 7 个保留编码，所以真实 swap 区上限约 25 个。原书说"32"只看到了位宽，没看到保留类型的分流。

**Q3：`swap_map[]` 数组存的是什么？为什么要 vmalloc 分配？**
A：每个 swap slot 一个 `unsigned char`，记录该 slot 的**引用计数**（有几个 PTE 指向这个 slot），特殊值有 `SWAP_MAP_MAX`（0x3e 最大计数）、`SWAP_MAP_BAD`（0x3f 坏块）、`SWAP_MAP_SHMEM`（0xbf shmem 独占）、`SWAP_HAS_CACHE`（0x40 有 cache）。它大小 = slot 总数，大分区可能几百万项、远超单页，所以用 `vmalloc` 分配（连续虚拟、物理散页）。

**Q4：`SWP_SOLIDSTATE` 和 `SWP_SYNCHRONOUS_IO` 分别代表什么？为什么它们是"现代"标志？**
A：`SWP_SOLIDSTATE` 标记 SSD——seek 廉价，于是分配策略从"顺序簇"改成"散开 + per-CPU"（§3）；`SWP_SYNCHRONOUS_IO` 标记 zram 这类设备——同步 I/O 反而比异步块层快（省去排队/中断开销），写回路径走 `swap_writepage_bdev_sync`（§5）。两者都是 2.6 之后才随 SSD/zram 普及出现的。

**Q5：`/proc/swaps` 里的 Size/Used/Priority 分别来自哪些字段？**
A：Size ← `pages`（可用页总数），Used ← `inuse_pages`（已用页数），Priority ← `prio`。都是 `swap_info_struct` 的直接字段，proc 接口只是把它们格式化输出。

</details>
