# Ch 2 §1 内存节点 (Nodes)

> **Understanding the Linux Virtual Memory Manager** · Mel Gorman · **精读 🔴**
> 源码核验：Linux **v6.6**（`include/linux/mmzone.h`）

---

## 本节讲什么

内存管理的**顶层容器**。本节回答三个问题：

1. NUMA / UMA 到底差在哪，为什么内核要抽象出「节点」这一层？
2. 一个节点用什么数据结构描述——`pglist_data` 的字段级真身是什么？
3. 分配内存时，内核怎么在**多个节点之间**选（zonelist 回退顺序）？

原书以 2.4/2.6 语境的 `pg_data_t` 叙述；v6.6 中它是 `typedef struct pglist_data`（`mmzone.h:1261`），字段已随 NUMA、内存热插拔、延迟初始化大幅演进，但「Node → Zone → Page」三层直觉不变。

---

## 1. NUMA 与 UMA

| 架构 | 全称 | 含义 | 访问特性 |
|------|------|------|----------|
| **NUMA** | Non-Uniform Memory Access | 大型机上内存按 CPU/socket 分**多个节点（Bank）** | CPU 访问**本地节点**快，**远端节点**慢（跨 socket 走互联总线，如 Intel UPI / AMD Infinity Fabric） |
| **UMA** | Uniform Memory Access | 传统 PC / 小服务器 | 整片内存视为**单一节点**，访问成本一致 |

VM 子系统把每个这样的内存组称为一个 **节点 (Node)**，用 `pglist_data` 描述。UMA 机器上只有一个节点（传统名 `contig_page_data`）；NUMA 机器每个 socket/内存控制器一个节点。

**为什么本地快、远端慢？** 现代 x86_64 每个 socket 有独立内存控制器，CPU 访问挂在自己 socket 下的 DIMM 走**本地内存控制器**，访问别的 socket 的内存要经过**跨 socket 互联链路**——多一跳、共享带宽、延迟更高。这正是 `numactl` 里 `node distances` 表（如 `10 / 21`）量化的东西。

---

## 2. `struct pglist_data` 真身（v6.6 `mmzone.h:1261`）

```c
typedef struct pglist_data {
    /* 本节点的 zone 数组（不含别的节点），可能部分未填充 */
    struct zone node_zones[MAX_NR_ZONES];

    /* 跨【所有节点的所有 zone】的引用表：分配时的回退顺序 */
    struct zonelist node_zonelists[MAX_ZONELISTS];

    int nr_zones;               /* 本节点实际填充的 zone 数 */
#ifdef CONFIG_FLATMEM
    struct page *node_mem_map;  /* 本节点 struct page 数组起点（FLATMEM） */
#endif
    unsigned long node_start_pfn;      /* 本节点第一个物理页框号 */
    unsigned long node_present_pages;  /* 实际存在的物理页总数（不含空洞） */
    unsigned long node_spanned_pages;  /* 地址范围跨度（含空洞） */
    int node_id;
    wait_queue_head_t kswapd_wait;     /* kswapd 回收守护进程的等待队列 */
    struct task_struct *kswapd;        /* 本节点的回收守护进程（Ch 10） */
    unsigned long totalreserve_pages;  /* 本节点保留、不给用户态分配的页 */
    struct lruvec __lruvec;            /* LRU 链表向量（Ch 10 回收的地基） */
    /* ... */
} pg_data_t;
```

字段分三组看：

| 字段 | 组 | 作用 |
|------|-----|------|
| `node_zones[]` / `nr_zones` | **结构** | 本节点到底有哪些 zone、有几个 |
| `node_zonelists[]` | **分配** | 分配内存时按什么顺序找 zone（跨节点回退） |
| `node_start_pfn` / `node_present_pages` / `node_spanned_pages` | **边界** | 本节点管辖的物理地址范围（有空洞时 spanned > present） |
| `node_mem_map` | **页数组** | FLATMEM 下本节点 `struct page` 数组首指针 |
| `kswapd` / `kswapd_wait` / `__lruvec` | **回收** | 每个节点一个回收线程 + 自己的 LRU，与 Ch 10 直接挂钩 |
| `totalreserve_pages` | **保留** | 系统应急页，用户态拿不到，防止 OOM 时无米下锅 |

**关键直觉：节点是「结构 + 分配策略 + 回收」三合一的自洽单元。** 它不仅描述物理内存长什么样，还决定了从哪分配、由谁回收。

---

## 3. 节点寻址：`NODE_DATA()` 与边界宏

内核用 `NODE_DATA(nid)` 拿节点指针（架构相关：x86_64 是 `node_data[]` 数组，UMA 折叠为 `&contig_page_data`）。配套的边界宏（`mmzone.h:1393-1399`）：

```c
#define node_present_pages(nid) (NODE_DATA(nid)->node_present_pages)
#define node_spanned_pages(nid) (NODE_DATA(nid)->node_spanned_pages)
#define node_start_pfn(nid)     (NODE_DATA(nid)->node_start_pfn)
#define node_end_pfn(nid)       pgdat_end_pfn(NODE_DATA(nid))  /* start + spanned */
```

```
物理地址空间（PFN 递增）
├─ Node 0 ──────────────┬─ Node 1 ──────────────
│  node_start_pfn=0      │  node_start_pfn=0x100000
│  spanned = 0x100000    │  spanned = 0x100000
│  present = 0xFE000     │  present = 0xFF800
│  （有内存空洞）          │
└───────────────────────┴───────────────────────
    每 node 内部再分 Zone（见 §2）
```

---

## 4. `node_zonelists`：分配时的跨节点回退

这是最容易混淆的一对字段：

| | `node_zones[]` | `node_zonelists[]` |
|---|---|---|
| 内容 | **本节点自己**的 zone | 引用**所有节点**的 zone 的**顺序表** |
| 谁在维护 | 启动/热插拔时填充 | `build_all_zonelists()`（`mmzone.h:1424` 声明）构建 |
| 干什么用 | 描述「我有哪些内存」 | 描述「分配时先找谁、再找谁」 |

`build_all_zonelists()` 按 **NUMA 距离（node distance）** 排序：本节点的 zone 排最前，距离越远的节点排越后。于是 `__alloc_pages()` 的默认行为天然是 **本地优先、远端兜底**：

```
Node 0 上某 CPU 请求页 → 查 node_zonelists[0]
    ├─ ① Node 0 的 ZONE_NORMAL / DMA32 ...（本地，先试）
    ├─ ② Node 1 的 ZONE_NORMAL ...          （距离 21，次选）
    └─ ③ ...                                 （更远节点）
```

**HFT 直接相关：** 一旦本地 zone 水位告急，分配会「静默」落到远端节点——之后每次访问都吃跨 socket 延迟。这就是为什么延迟敏感程序要 `numactl --membind` **强制绑死本地节点**（而不是默认的 `--interleave` 或 localonly 回退）。

---

## 5. 代码示例

**① 看机器有几个节点、距离多少：**

```bash
$ numactl --hardware
available: 2 nodes (0-1)
node 0 cpus: 0 1 2 3
node 1 cpus: 4 5 6 7
node distances:
node   0   1
  0:  10  21     # 本地 10，跨 socket 21
  1:  21  10
```

**② 节点在 sysfs 下的真实形态：**

```bash
$ ls /sys/devices/system/node/
node0  node1  online  possible  has_cpu  has_memory

$ cat /sys/devices/system/node/node0/meminfo
Node 0 MemTotal:       32768000 kB
Node 0 MemFree:        16384000 kB
```

**③ 用户态强制本地分配：**

```c
#include <numa.h>
/* 把本线程后续内存分配绑到 node 0，禁止回退 */
numa_set_preferred(0);          /* 首选，可回退 */
numa_set_membind(numa_no_nodes_ptr ? 0 : 0);  /* 强制，无回退（需 numa_bind） */
```

---

## 6. HFT / 嵌入式关联

| 现象 | 本节机制的兑现 |
|------|----------------|
| 跨 socket 访问延迟翻倍 | node distance `10 vs 21`；`numactl --membind` 把分配钉死本地 |
| 订单簿内存池「绑核绑内存」 | 每核的数据结构分配到**该核所在 node**，避免远端访存 |
| NIC 收包缓冲与 CPU 不在同 node | 网络栈收包页落在 NIC 所在 node，处理它的 CPU 若在别的 node 则全程跨 socket |
| `mlock` 后仍可能抖动 | `mlock` 防换出，但**不防**本地 zone 告急触发回退到远端 node |

---

## 7. 衔接

- 下节 [§2 内存区域](./section-2-内存区域.md)：节点内部再分 Zone 与水位
- [§3 物理页框](./section-3-物理页框.md)：`node_mem_map` 里的 `struct page`
- 回收：[Ch 10 页框回收](../../chapter-10-page-frame-reclamation/)（`kswapd` 落地处）
- 现代演进：[06.5/ch01 物理内存与 memblock](../../../06.5-modern-mm/chapter-01-physical-memory-memblock/)

---

<details>
<summary>代码自测（Q&A）</summary>

**Q1：UMA 机器上也有 `pglist_data` 吗？**
A：有。UMA 退化为**只有一个节点**（`contig_page_data`），`NODE_DATA(0)` 就是它。整个 NUMA 抽象在 UMA 上仍然成立，只是「多节点」退化成「单节点」，跨节点回退逻辑从不触发。这跟「单 CPU 机器也有 per-CPU 变量」是同一类设计——**抽象不因硬件退化而消失**。

**Q2：`node_spanned_pages` 和 `node_present_pages` 为什么是两个数？**
A：物理地址空间可能有**空洞**（内存条没插满、BIOS 保留、内存热插拔留下间隙）。`spanned` 是地址范围跨度（含洞），`present` 是实际存在的物理页（不含洞）。回收器/分配器用 `present`，边界计算用 `spanned`。

**Q3：分配内存时内核「静默回退到远端节点」，怎么观察到？**
A：`numastat` 看每个节点的 `numa_hit` / `numa_miss` / `numa_foreign` 计数。`numa_miss` 增长说明有分配落在非本节点；也可以 `cat /sys/devices/system/node/node*/numastat`。

**Q4：`numactl --membind` 和 `mbind()` 系统调用什么关系？**
A：`numactl` 是用户态工具，底层调 `set_mempolicy()` / `mbind()`（前者改当前线程的默认策略，后者改一段已分配内存的归属）。`--membind` 对应 `MPOL_BIND`——**严格绑定**，不允许回退，若该节点内存不够则分配失败，而不是静默落到远端。

**Q5：为什么 `node_zonelists` 是「所有节点的 zone 的引用」而不是每个节点各维护一份自己的？**
A：分配发生在**任意 CPU**上，而该 CPU 需要知道「整个系统」的可用 zone 按距离排序后的顺序。把回退顺序**冗余存储在每个节点的 `pglist_data` 里**，分配路径只查自己节点的 `node_zonelists`，不必跨节点读别的节点的数据结构——**省一次跨 socket 访存**，也避免锁竞争。

</details>

---
