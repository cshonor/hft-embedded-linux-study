# Ch 5 §1 启动内存映射的表示 (`bootmem_data` → `struct memblock`)

> **Understanding the Linux Virtual Memory Manager** · Mel Gorman · **跳过 ⚪**
> 源码核验：Linux **v6.6**（`include/linux/memblock.h` / `mm/memblock.c`）

---

## 本节讲什么

本节回答一个核心问题：**内核启动早期，还没有 Buddy/slab，用什么数据结构记录「哪些物理内存可用、哪些被占用」？**

原书答案是 `bootmem_data`（**位图**，每一位对应一个页框）；但 **v6.6 里 bootmem 已彻底消失，取代它的是 `struct memblock`（区间表）**。本节先讲原书的位图方案，再落到 v6.6 的真身——这是理解「为什么 memblock 更快」的关键。

---

## 1. 原书方案：`bootmem_data` 位图（已删除）

**每个内存节点 (Node)** 用 `bootmem_data` 记录分配状态：

| 字段 | 作用 |
|------|------|
| `node_bootmem_map` | 指向**位图**——每一位对应一个物理页框：0=空闲 / 1=已分配 |
| `last_pos` | 上次分配用到的 PFN（bump-pointer 起点） |
| `last_offset` | 该页内上次分配结束的偏移 |

```
bootmem_data (per node)
    node_bootmem_map ──► [ bit per page frame: 0=free 1=used ]
    last_pos / last_offset ──►  bump-pointer 式同页续分配
```

**位图方案的本质缺陷**：一个 4GB 内存的节点，位图要 4G/4K = 1M 位 = 128KB 内存，但**更大的问题是「找空闲页」要线性扫位**——`last_pos` 只是缓解，复杂度仍是 O(n)。这在内存极大的 NUMA 机器上（几十 TB）是不可接受的。

---

## 2. v6.6 真身：`struct memblock`（区间表）

v6.6 里，启动分配器是 **memblock**，用**有序区间数组**取代位图（`memblock.h:59-98`）：

```c
/* include/linux/memblock.h */
struct memblock_region {                 /* :59 单个区间 */
    phys_addr_t base;                    /* 起始物理地址 */
    phys_addr_t size;                    /* 大小 */
    enum memblock_flags flags;           /* 属性（见下表） */
#ifdef CONFIG_NUMA
    int nid;                             /* 所属 NUMA 节点 */
#endif
};

struct memblock_type {                   /* :76 一类区间的集合 */
    unsigned long cnt;                   /* 当前区间个数 */
    unsigned long max;                   /* regions 数组容量 */
    phys_addr_t total_size;              /* 所有区间总大小 */
    struct memblock_region *regions;     /* 区间数组（按 base 排序） */
    char *name;                          /* "memory" / "reserved" */
};

struct memblock {                        /* :91 全局唯一的分配器元数据 */
    bool bottom_up;                      /* 自底向上还是自顶向下分配 */
    phys_addr_t current_limit;           /* 当前分配地址上限 */
    struct memblock_type memory;         /* 可用内存区间 */
    struct memblock_type reserved;       /* 已保留区间 */
};

extern struct memblock memblock;         /* :98 全局单例 */
```

| 字段 | 作用 |
|------|------|
| `memblock.memory` | **系统有哪些可用物理内存**（firmware/e820/DT 报告后 `add` 进来） |
| `memblock.reserved` | **其中哪些被占用**（内核代码、initrd、crashkernel、页表…） |
| `bottom_up` | 分配方向（`true`=自底向上；`false`=自顶向下，避 shadow/RAM 洞） |
| `current_limit` | 分配地址不能超过它（早期常限在低 4GB，DMA 可达） |

**关键直觉：memblock 是「两表模型」。** `memory` 表记录「有什么」，`reserved` 表记录「哪些不能给」。分配时 = 在 `memory` 里找一段**不在 `reserved` 里**的区间。这比 bootmem 的「单张全量位图」多了**天然的区域语义**（一段一段的区间，而非逐页的 bit）。

---

## 3. 区间属性：`enum memblock_flags`

| 标志（`memblock.h:44-48`） | 值 | 含义 |
|------|----|------|
| `MEMBLOCK_NONE` | `0x0` | 普通区间，无特殊要求 |
| `MEMBLOCK_HOTPLUG` | `0x1` | **可热插拔**内存（firmware 标记，内存条可带电拔插） |
| `MEMBLOCK_MIRROR` | `0x2` | **镜像**内存（容错，数据写两份） |
| `MEMBLOCK_NOMAP` | `0x4` | **不进内核直接映射**（如保留给设备的物理区间） |

这些 flag 在「区间」粒度上打标，是位图方案（只能表达 free/used 二元态）**表达不了的语义**。NUMA 上还有 `nid` 字段，一个区间直接带上「属于哪个节点」——这也是 Ch2 §1 `pglist_data` 的 `node_start_pfn` 边界信息**最初的来源**。

---

## 4. 位图 vs 区间表：为什么 memblock 胜出

| | bootmem 位图 | memblock 区间表 |
|---|-------------|----------------|
| 数据结构 | 每页 1 bit | 有序 `memblock_region[]` 数组 |
| 找空闲块 | **线性扫位** O(n) | 二分/顺序遍历区间，O(区间数) |
| 内存开销 | 与总内存成正比（4G→128KB） | 与**区间个数**成正比（通常几十个，几 KB） |
| 区域语义 | 只有 free/used | `flags`（hotplug/mirror/nomap）+ `nid` |
| 大内存扩展性 | 差（TB 级位图扫描极慢） | 好（区间数几乎不随内存量增长） |
| v6.6 状态 | **已删除** | 唯一启动分配器 |

**为什么 bootmem 被废？** 一句话：**内存越大、NUMA 越复杂，逐页位图就越慢、越表达不了语义**。memblock 的「区间 + 属性」模型天生适配大内存和 NUMA，最终在 v4.20 前后把 bootmem 彻底替换（Mike Rapoport 主导的 memblock 统一）。

---

## 5. 衔接

- 下节 [§2 发现与初始化](./section-2-发现与初始化.md)：`memory`/`reserved` 两表怎么被填满
- 节点边界：[Ch2 §1 内存节点](../../chapter-02-describing-physical-memory/notes/section-1-内存节点.md)（`node_start_pfn` 的来源）

---

<details>
<summary>代码自测（Q&A）</summary>

**Q1：bootmem 的位图和 memblock 的区间表，最本质的区别是什么？**
A：**粒度不同**。位图是「每页 1 bit」的**逐页**表示，找空闲要线性扫位；memblock 是「一段区间」的**区域**表示，一段内存一个 `{base, size, flags, nid}` 记录。前者内存开销随总内存线性增长，后者只随「区间个数」增长——大内存 + NUMA 下差距悬殊。

**Q2：`memblock.memory` 和 `memblock.reserved` 分别回答什么问题？**
A：`memory` 答「**系统有什么**」——firmware/e820/DT 报告的可用物理内存；`reserved` 答「**其中哪些不能给**」——内核代码、initrd、crashkernel、页表等占用。分配 = 在 `memory` 里找一段**不被 `reserved` 覆盖**的区间。两表分离让「保留」从「打位」变成「加区间」，语义更清晰。

**Q3：`bottom_up` 分配方向有什么用？**
A：`bottom_up = true` 时从低地址往高找，`false` 时从高往低。早期要避开低端 DMA 区/BIOS shadow 就自顶向下；要配合某些固件把内核放低位就自底向上。它本质是**分配策略的一个开关**，控制 memblock 在 `memory` 区间里**往哪头找**。

**Q4：`MEMBLOCK_NOMAP` 和「reserved」有什么不同？**
A：`reserved` 表示「这段被占用，别分配给别人」；`MEMBLOCK_NOMAP` 是更进一步的属性——这段内存**不建立内核直接映射**（`__va` 访问不到），典型是保留给 PCIe 设备/特殊硬件的物理区间。两者可叠加：一段区间既 reserved 又 NOMAP。

**Q5：为什么说 memblock 的 `nid` 字段是 Ch2 `node_start_pfn` 的「最初来源」？**
A：启动早期 NUMA 还没建 `pglist_data`，但 firmware（ACPI SRAT / DT）已经报告了「哪段内存属于哪个节点」。memblock 在 `add` 区间时把这个 nid 记下来，之后 `free_area_init`（Ch2 节点/zone 初始化）就**读 memblock 的区间 + nid** 反推出每个节点的 `node_start_pfn` / `node_present_pages`。

</details>

---
