# Ch 2 描述物理内存 · Describing Physical Memory

> **Understanding the Linux Virtual Memory Manager** · Mel Gorman · **精读**

Linux 支持多种硬件体系结构，内核需要 **与体系结构无关** 的方式描述物理内存。本章按 **宏观 → 微观** 展开：**节点 (Nodes) → 区域 (Zones) → 物理页框 (Pages)**。

> **时代说明：** 原书以 **2.4 / 2.6** 内核为主（`zone_t`、`struct zone` 等命名）。现代主线仍保留 **Node / Zone / struct page** 三层直觉，具体字段与 LRU、per-CPU 页缓存机制在后续版本继续演进 — 读源码以 [Elixir Bootlin](https://elixir.bootlin.com/linux/latest/source) 为准。

---

## 本章在 VM 子系统中的位置

```
物理内存怎么「分块、分档、分状态」描述清楚
        ↓
Ch 3 页表：虚拟地址如何映射到这些页框
        ↓
Ch 4 VMA：进程地址空间如何引用这些结构
        ↓
Ch 6 page_alloc / Ch 10 回收：在这些 Zone 上分配与回收
```

**HFT 为什么要先读本章：** **NUMA 本地分配**、**Zone 水位与回收延迟**、**struct page 状态**、**缓存行对齐（padding）** — 都直接影响 **绑核绑内存、mlock、大页、内存池布局** 时「为什么会抖、为什么会慢」。

→ 源码入口（Ch 1 路线第 3 步）：[`mm/page_alloc.c`](https://elixir.bootlin.com/linux/latest/source/mm/page_alloc.c) · 相关 [`mm/mmzone.c`](https://elixir.bootlin.com/linux/latest/source/mm/mmzone.c)

---

## 1. 内存节点 (Nodes)

### NUMA 与 UMA

| 架构 | 含义 | 访问特性 |
|------|------|----------|
| **NUMA**（Non-Uniform Memory Access） | 大型机器上内存分 **多个 Bank/节点** | CPU 访问 **本地节点** 快，**远端节点** 慢（距离不同） |
| **UMA**（Uniform Memory Access） | 传统 PC / 小服务器 | 视为 **单一节点**，访问成本一致 |

VM 子系统把 **每个内存组** 称为一个 **节点 (Node)**。

### `pg_data_t`

内核用 **`pg_data_t`**（现代树中常与 **`struct pglist_data`** 对应）描述一个节点：

| UMA 系统 | NUMA 系统 |
|----------|-----------|
| 整片物理内存 ≈ **一个节点** | 多个 `pg_data_t`，各有 zone、页框 |
| 传统名 **`contig_page_data`** | 每 socket / 每 NUMA node 一个 |

**节点是物理内存的顶层容器** — 其下再划 Zone，再落到 `struct page`。

→ HFT：**`numactl --membind` / `mbind()`** 就是在 **指定 Node 上分配** — 跨 node 访问 = 额外延迟与带宽争用（→ [03-SysPerf Ch7](../03-Systems-Performance-2nd/chapter-07-memory/) · Hennessy 内存层次）。

---

## 2. 内存区域 (Zones)

每个 **Node** 再划分为多个 **区域 (Zone)**。典型 x86 32 位划分：

| Zone | 用途（简） |
|------|------------|
| **`ZONE_DMA`** | 低端物理地址，供 **ISA DMA** 等必须落在有限物理范围的设备 |
| **`ZONE_NORMAL`** | 内核 **直接线性映射** 可常态访问的「普通」物理页 |
| **`ZONE_HIGHMEM`** | **32 位** 上超出内核直接映射窗口的物理内存（见 §4） |

每个 Zone 由 **`zone_t`**（2.6 起 **`struct zone`**）描述。

### 区域水位线 (Zone Watermarks)

页面 **分配** 与 **回收** 的触发器 — 每个 Zone 有三个关键水位：

| 水位 | 含义 | 典型后果 |
|------|------|----------|
| **`pages_high`** | 空闲页较多 | 正常分配 |
| **`pages_low`** | 空闲页偏少 | 唤醒 **`kswapd`** 后台回收 |
| **`pages_min`** | 空闲页紧张 | 分配路径上 **同步回收**（**direct reclaim**）— 调用方可能被拖慢 |

```
空闲页数量
    high ─────────────────  正常
     low ─────────────────  kswapd 开始干活
     min ─────────────────  分配器同步回收（延迟尖刺）
```

**HFT 关联：** 即使用 `mlock`，系统其他部分仍可能因 **全局内存压力** 触发回收路径；绑 NUMA + 预分配 + 监控 **direct reclaim**（`/proc/vmstat` 等）是常见调优项。

### 等待队列表 (Wait Queue Table)

页在进行 **I/O**（换入/换出、块设备读写）时会被 **锁定**（`PG_locked`）。等该页的进程需 **睡眠**。

- **不为每个 page 单独建等待队列**（太费内存）
- 在 **Zone** 里维护 **哈希形式的 wait queue table** — 多个 page 共享桶式等待结构

→ 读回收 / swap 时会反复碰到：**页 locked → 在 zone 等待队列上 sleep → I/O 完成唤醒**。

---

## 3. 物理页框 (Pages)

系统中 **每个物理页框** 对应一个 **`struct page`** — 内核用它 **持续跟踪** 该页框状态（无论是否已映射到用户空间）。

### 核心字段（原书强调）

| 字段 | 作用 |
|------|------|
| **`mapping`** | 若页属于 **文件映射**，指向该文件的 **address space**；匿名页 / slab 等另有用法 |
| **`count`** | **引用计数**；`0` → 可释放；`>0` → 正被进程或内核使用 |
| **`flags`** | 描述页当前状态的 **标志位集合** |

### 页面状态标志 (Page Flags) — 示例

| 标志 | 含义 |
|------|------|
| **`PG_active`** | **热页** — 在 **active LRU** 链上，近期被访问 |
| **`PG_dirty`** | 页内容已改，需 **写回** 磁盘 |
| **`PG_locked`** | 正被 **I/O 锁定** |
| **`PG_uptodate`** | 已从磁盘 **成功读入**，内容有效 |

LRU、dirty、locked 等标志与 **Ch 10 页框回收**、**swap** 直接相连 — 回收器决定 **踢哪一页**，就是看 **哪个 zone 的哪条 LRU、什么 flags**。

→ 用户态 `mmap` 的文件页、匿名页，最终都落实为 **`struct page` + 引用计数 + flags**（→ [08-TLPI](../08-The-Linux-Programming-Interface/) · [05-LKD Ch12](../05-Linux-Kernel-Development/00_Book_3rd_Notes/chapter-12-memory-management/)）。

---

## 4. 高端内存 (High Memory)

**32 位 x86** 上，内核 **直接映射** 的虚拟地址窗口有限（`ZONE_NORMAL` 能覆盖的物理范围有顶）。

| 问题 | 内核做法 |
|------|----------|
| 物理内存在 **1GiB–4GiB**（甚至 **PAE 下更大**，如 64GiB） | 落在 **`ZONE_HIGHMEM`** |
| 内核不能随时用「线性偏移」访问 | 需 **`kmap()`** 等 **临时映射** 到 `ZONE_NORMAL` 可访问的虚拟地址，用完 **`kunmap()`** |

**64 位** 桌面/服务器上 **HIGHMEM 常不存在或为空** — 但 **「并非所有物理页都能零成本直接 touch」** 的思想仍在（IO 映射、特殊区域等）。

→ 与 [Ch 9 高端内存管理](./chapter-09-高端内存管理.md)（原书专章，现代 x86_64 可读作背景）。

---

## 5. 2.6 内核的新变化 (What's New in 2.6)

原书末尾强调 2.6 相对 2.4 的三点 — **对多核性能理解仍有用**：

### LRU 链表本地化

| 2.4 | 2.6+ |
|-----|------|
| **`active_list` / `inactive_list` 全局** | LRU 链 **移入各 `struct zone` 内部** 维护 |
| 回收顺序全局竞争 | **按 zone 局部** 决定回收 — 更贴合 NUMA / 多 zone 现实 |

→ 接 [Ch 10 页框回收](./chapter-10-页框回收.md)。

### 每 CPU 页面集合 (Per-CPU Page Lists · `pageset`)

| 问题 | 2.6 做法 |
|------|----------|
| 多 CPU **同时** 从 zone  freelist 取页 → **`zone->lock` 争用** | 在 **`struct zone`** 里为 **每个 CPU** 维护 **热/冷页缓存**（`pageset`） |
| 频繁自旋锁 | 多数分配 **先命中 per-CPU 列表**，减少锁竞争 |

**HFT 关联：** 与 **per-CPU 变量、无锁热路径** 同一思路 — 把 **共享锁上的操作** 变成 **本地缓存 + 批量 refill**（→ DPDK mempool、订单簿 per-core 分配区）。

### 结构体填充 (Padding · 缓存行隔离)

2.6 在 **`struct zone`** 中增加 **padding**，使 **`zone->lock`** 与 **`zone->lru_lock`** 等 **高频成对访问的锁** 落在 **不同 cache line** 上。

→ **false sharing / 缓存一致性流量** — 与 [Hennessy Ch2](../02-Computer-Architecture-6th/chapter-02-memory-hierarchy-design/) · HFT **订单簿/cache line 对齐** 同构。

---

## 三层结构一图

```
                    ┌─────────────────────────────────┐
                    │  Node (pg_data_t)               │
                    │  NUMA node 0, 1, …              │
                    └───────────────┬─────────────────┘
                                    │
              ┌─────────────────────┼─────────────────────┐
              ▼                     ▼                     ▼
        ┌───────────┐       ┌───────────┐       ┌───────────┐
        │ ZONE_DMA  │       │ZONE_NORMAL│       │ZONE_HIGHMEM│
        │ watermarks│       │ LRU lists │       │ (32-bit)  │
        │ wait table│       │ pageset   │       └───────────┘
        └─────┬─────┘       └─────┬─────┘
              │                   │
              └─────────┬─────────┘
                        ▼
              每个物理页框 → struct page
              (mapping, count, flags: active/dirty/locked/…)
```

---

## HFT 精读 checklist

| 概念 | 落地 |
|------|------|
| **Node** | 进程/线程 **绑 NUMA node** 分配；订单簿 **与 NIC 同 node** |
| **Zone watermarks** | 监控 **direct reclaim**；避免与延迟敏感线程争用内存 |
| **`struct page` / LRU** | 理解 **mlock** 钉住的是哪些页；THP 是 **compound page** 叠加在此之上 |
| **per-CPU pageset** | 内核侧「**每核缓存减少锁**」— 用户态 mempool 设计的内核版镜像 |
| **zone padding** | **锁/热字段分 cache line** — 用户态结构体同样要做 |

---

## 相关章节

- 上一章：[chapter-01-简介.md](./chapter-01-简介.md)
- 下一章：[chapter-03-页表管理.md](./chapter-03-页表管理.md)
- 附录 B（Code Commentary）：[appendix-B-描述物理内存.md](./appendix-B-描述物理内存.md)
- 交叉：[05-LKD Ch12 内存管理](../05-Linux-Kernel-Development/00_Book_3rd_Notes/chapter-12-memory-management/) · [06-ULK Ch8](../06-Understanding-Linux-Kernel/chapter-08-内存管理.md) · [01-CSAPP Ch9](../01-CSAPP-3rd/chapter-09-virtual-memory/)

---
