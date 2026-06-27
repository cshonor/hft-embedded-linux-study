# Ch 5 启动内存分配器 · Boot Memory Allocator

> **Understanding the Linux Virtual Memory Manager** · Mel Gorman · **跳过**（HFT 热路径 **不经过** bootmem；读作 **boot → 伙伴系统**  handoff 背景即可）

## 本章解决什么问题

内核启动时的 **「鸡生蛋」**：

```
要初始化 物理页分配器（伙伴系统, Ch 6）
    → 需要分配内存来放它的数据结构
        → 但伙伴系统 还没就绪 …
```

在 **正式物理页分配器**（**Buddy Allocator**）完全可用之前，内核使用 **临时、结构简单** 的 **启动内存分配器 (Boot Memory Allocator)** 为 **boot 阶段** 的数据结构分配内存 — **包括伙伴系统自身初始化所需的内存**。

> **时代说明：** 原书描述 **`bootmem` / `bootmem_data`**（2.4–2.6 常见）。**现代主线** 已 largely 由 **`memblock`** 取代（bootmem 代码已移除）；**角色相同** — early alloc → **`mem_init()` 退役** → 页框交给 **`page_alloc.c` 伙伴系统**。读当前源码见 [`mm/memblock.c`](https://elixir.bootlin.com/linux/latest/source/mm/memblock.c) · [`mm/init-mm.c`](https://elixir.bootlin.com/linux/latest/source/mm/init-mm.c) 等。

---

## 本章在 VM 子系统中的位置

```
Ch 3 pagetable_init / paging_init  （需要 early 内存）
        ↓
Ch 5 bootmem / memblock            ← 本章：临时位图分配器
        ↓
Ch 6 page_alloc.c · 伙伴系统       ← boot 结束 handoff 目标
        ↓
Ch 4 运行时 fault / mmap          ← 进程用的已是 Buddy + slab 世界
```

**HFT：** 生产路径 **从不** 调用 `alloc_bootmem`；理解本章有助于 **读 dmesg / 启动参数 / 内存布局**，以及 **「第一个真正 page 从哪来」**。

---

## 1. 启动内存映射的表示 (`bootmem_data`)

**每个内存节点 (Node)** 在启动阶段用 **`bootmem_data`**（或 **`bootmem_data_t`**）记录分配状态：

| 字段 | 作用 |
|------|------|
| **`node_bootmem_map`** | 指向 **位图 (bitmap)** — **每一位对应一个物理页框**：空闲 / 已分配 |
| **`last_pos`** | 上次分配用到的 **物理页框号 (PFN)** |
| **`last_offset`** | 该页内 **上次分配结束的偏移** |

**合并小额分配：** 新的小块分配若 **仍能塞进 `last_pos` 那一页** 的剩余空间，则 **与上次同页合并** — 减少 **整页占用**（boot 阶段内存也珍贵）。

```
bootmem_data (per node)
    node_bootmem_map ──► [ bit per page frame: 0=free 1=used ]
    last_pos / last_offset ──►  bump-pointer 式同页续分配
```

→ 与 Ch 2 **Node / PFN / struct page** 同一套物理页框编号。

---

## 2. 发现与初始化 (Initializing)

### 架构相关探测

在 **arch 相关 setup** 阶段，内核探测 **可用物理内存边界**，得到例如：

| 参数 | 含义 |
|------|------|
| **`min_low_pfn`** | 最低可用页框号 |
| **`max_low_pfn`** | **NORMAL 区** 最高可用页框号 |
| **高端内存 PFN 范围** | 32 位 **HIGHMEM** 起止（Ch 2 §4） |

### `init_bootmem_core()`

| 步骤 | 做什么 |
|------|--------|
| 初始化 **`bootmem_data`** |  per-node 启动分配器状态 |
| 计算 **位图大小** | 覆盖该 node 全部页框 |
| **为位图本身分配内存** | 仍用 boot 机制（早期可能 **静态/预留区**） |
| **位图初始化为「全保留」** | 再逐步 **free** 可用物理范围 |

**直觉：** 先 **假设全被占用**，再把 **固知空闲的物理 RAM** 标成可分配 — 与 **memblock reserve/free** 现代逻辑同构。

---

## 3. 内存分配与释放 (Alloc / Free)

### 分配 API（UMA / NUMA 两套，语义相近）

| 宏 / 函数（原书） | 典型用途 |
|-------------------|----------|
| **`alloc_bootmem()`** | 分配 **boot 生命周期内** 的内核数据结构 |
| **`alloc_bootmem_low()`** | 倾向从 **低端 / DMA 可达** 物理区分配 |
| **`alloc_bootmem_pages()`** | 按 **整页** 粒度分配 |

NUMA 变体带 **node 参数** — 在 **指定 node** 上分配。

### 释放的限制

**`free_bootmem()`** 重要约束：

| 规则 | 后果 |
|------|------|
| **只能释放完整页** | 页内 **部分占用** 时，bootmem **不跟踪** 页内碎片 |
| 若只「部分释放」 | 整页仍视为 **保留** |

**为何可接受：** boot 阶段分配的内存 **几乎伴随系统一生**（页表、mem_map、伙伴系统元数据…）— **很少真正 free**，退役时 **整页移交 Buddy** 即可。

---

## 4. 启动内存分配器的退役 (Retiring)

当 **`start_kernel()` 路径后期** 可以安全运行 **常规分配器** 时，**bootmem 退役**：

| 步骤 | 说明 |
|------|------|
| 各 arch 提供 **`mem_init()`** | 架构相关的收尾 + 调用通用逻辑 |
| **遍历 bootmem 位图** | 找出 **仍标记为空闲** 的物理页 |
| **清除保留标记** | 含 **保存位图本身** 的页 — 也还给运行时 |
| **全部交给 Buddy** | **`page_alloc.c`** 构建 **zone freelist** — Ch 6 正式接管 |

```
bootmem 位图:  used used free free used …
                    └────────┘
                         ↓ mem_init()
              Buddy freelist (ZONE_NORMAL, …)
```

**之后：** `kmalloc`、`page_alloc`、`mmap` fault 等 **不再走 bootmem** — 用户态进程 **永远不会** 触达此路径。

→ **下一章精读：** [chapter-06-物理页分配.md](./chapter-06-物理页分配.md)

---

## 5. 2.6 内核的新变化

2.4 → 2.6 **bootmem 无架构级重写**，主要是 **小优化**：

| 变化 | 作用 |
|------|------|
| **`last_success` 字段**（`bootmem_data_t`） | 记录 **上次成功分配位置** — 缩短后续 **扫描位图找空闲位** 的距离 |

与 **`last_pos` / `last_offset`** 同类：**减少 boot 路径上 O(n) 位图扫描**。

---

## bootmem → Buddy 一图

```
        上电 / arch setup
              │
              ▼
    探测 PFN 范围 (min_low_pfn, max_low_pfn, …)
              │
              ▼
    init_bootmem_core · 位图 per node
              │
              ▼
    alloc_bootmem*  ──► 页表、mem_map、伙伴系统元数据 …
              │
              ▼
    mem_init() · 退役 bootmem
              │
              ▼
    空闲页框 ──► Buddy (Ch 6) ──► 此后所有 runtime 分配
```

---

## HFT / 阅读建议

| 读者 | 建议 |
|------|------|
| **HFT 工程** | **可跳过正文**；知道 **运行时内存来自 Buddy + slab**，bootmem 仅 **启动 几百毫秒** |
| **读内核启动 / 调试** | 理解 **memblock Reserve/Allocate** 与 **`/proc/iomem`** 中 **Kernel code / reserved** 从哪来 |
| **继续精读** | [Ch 6 物理页分配](./chapter-06-物理页分配.md) · Ch 1 路线中的 [`page_alloc.c`](https://elixir.bootlin.com/linux/latest/source/mm/page_alloc.c) |

---

## 相关章节

- 上一章：[chapter-04-进程地址空间.md](./chapter-04-进程地址空间.md)
- 下一章：[chapter-06-物理页分配.md](./chapter-06-物理页分配.md)
- 附录 E：[appendix-E-启动内存分配器.md](./appendix-E-启动内存分配器.md)
- Ch 3 引导页表：[chapter-03-页表管理.md](./chapter-03-页表管理.md#4-内核页表初始化-kernel-page-tables)

---
