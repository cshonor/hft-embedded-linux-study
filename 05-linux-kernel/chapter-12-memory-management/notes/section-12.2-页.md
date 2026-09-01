## ② 页 · Pages

物理内存管理的基本粒度是 **页（page frame）** — 通常 **4KB**（`PAGE_SIZE`），Huge page 另论。

#### `struct page` — 描述页框，不是页内数据

| 要点 | 说明 |
|------|------|
| **`struct page`** | **每个物理页框** 一个描述符 — 在 **mem_map[]** 或 **sparse vmemmap** 中 |
| **不是** 页内容 | 数据通过 **`page_address()` / kmap** 得到 **内核 VA** |
| **`page->flags`** | 脏、锁定、LRU、compound head 等 |
| **`page->_refcount`** | **引用计数** — 归零且不在 LRU 时可回收 |

```
物理 RAM 页框 #N
    │
    ├── struct page[N]   ← 元数据
    └── 4KB 数据          ← 通过线性映射或 kmap 访问
```

#### 页的状态（概念）

| 状态 | 含义 |
|------|------|
| **空闲** | 在 **伙伴系统** free list |
| **已分配** | refcount > 0 — 某子系统持有 |
| **缓存** | **页缓存**（Ch 16）— 可回收 |
| **Slab 页** | 切成固定对象槽 |

#### 与伙伴系统（Buddy）

| 阶 order | 含义 |
|----------|------|
| **order 0** | 1 页（4KB） |
| **order 1** | 2 页连续 |
| **order n** | 2^n 页连续 |

`alloc_pages(gfp, order)` 从 **合适 order** 链表取块；释放时 **尝试合并 buddy** 减碎片。

#### 伙伴系统的阶有上限（v6.6 实证）

```c
/* include/linux/mmzone.h:28 */
#ifndef CONFIG_ARCH_FORCE_MAX_ORDER
#define MAX_ORDER 10            /* v6.6 默认，注意不是 11 */
#endif
#define MAX_ORDER_NR_PAGES (1 << MAX_ORDER)
```

| 阶 | 连续物理页 | 大小（4KB 页） |
|----|-----------|---------------|
| order 0 | 1 | 4 KB |
| order 1 | 2 | 8 KB |
| order 5 | 32 | 128 KB |
| **order 10（上限）** | **1024** | **4 MB** |

> **单次 `alloc_pages(gfp, order)` 最多只能要 4MB 物理连续内存**——
> 这是伙伴分配器的硬上限（架构可以用 `CONFIG_ARCH_FORCE_MAX_ORDER` 改，但改大反而更容易失败）。
> 要更大的连续块，路径是 **启动时预留 / CMA / hugetlb**，不是运行时分配。

#### 反碎片：order 之外还有「迁移类型」

伙伴系统的 free list 是**二维**的：每个 order 再按 **迁移类型** 分链表
（`struct free_area` 里是 `free_list[MIGRATE_TYPES]`，`mmzone.h:112`）：

| 类型 | 放什么 | 能否搬移 |
|------|--------|---------|
| `MIGRATE_UNMOVABLE` | 内核核心数据（不可移动） | ✗ |
| `MIGRATE_MOVABLE` | 用户态匿名页、页缓存 | ✓ |
| `MIGRATE_RECLAIMABLE` | 可回收（如 inode/dentry 缓存） | 可回收 |
| `MIGRATE_HIGHATOMIC` | 高阶原子分配的紧急储备 | ✗ |
| `MIGRATE_CMA` | CMA 预留区（`CONFIG_CMA`） | 仅可移动页 |
| `MIGRATE_ISOLATE` | 隔离中（热插拔/CMA 迁移时） | 不可分配 |

**回落表**（`page_alloc.c:1597` 实证）—— 要的类型没页时，按这个顺序偷：

```c
static int fallbacks[MIGRATE_TYPES][MIGRATE_PCPTYPES - 1] = {
	[MIGRATE_UNMOVABLE]   = { MIGRATE_RECLAIMABLE, MIGRATE_MOVABLE   },
	[MIGRATE_MOVABLE]     = { MIGRATE_RECLAIMABLE, MIGRATE_UNMOVABLE },
	[MIGRATE_RECLAIMABLE] = { MIGRATE_UNMOVABLE,   MIGRATE_MOVABLE   },
};
```

> 设计意图：把**可移动的页聚在一起**，让 `MIGRATE_MOVABLE` 区域始终保持大块连续，
> 需要时通过迁移腾出连续空间（CMA、内存热拔、THP 都靠它）。
> 这就是"长时间运行后高阶分配失败"的真正解药——**不是靠运气，是靠分类隔离**。

#### 现代演进：`struct page` → `folio`

LKD 时代一切以 `struct page` 为中心。v5.16 起引入 **`struct folio`**，
v6.6 的内存管理主体代码（页缓存、回写、LRU）已大量改用 folio（详见 [Ch 16](../chapter-16-page-cache/)）：

| | `struct page` | `struct folio` |
|---|---|---|
| 语义 | **一片**页（compound page 的 tail 页也是 page） | **一组**物理连续的页（头页即 folio） |
| 问题 | 传一个 `page*` 不知道它是整个大页还是其中一片 | 类型层面消除歧义 |
| 现状 | 仍是底层元数据，页表/伙伴层仍用 | 高层（页缓存、回写、LRU）逐步 folio 化 |

> **注意**：`struct page` 的数量没变——4GB 内存 / 4KB = 100 万个 page 描述符。
> folio 是**同一块内存之上的另一种视图**（头页复用），不是新增元数据。

#### 嵌入式 / ARM 注意

| 点 | 说明 |
|----|------|
| **`PAGE_SIZE`** | 多数 ARM32/64 为 **4KB**；部分 **64KB** 配置 |
| **无 MMU** | **uClinux** 等 flat 模型 — 无 `struct page` 完整语义（本书以 MMU Linux 为主） |
| **CMA** | **Contiguous Memory Allocator** — 给 DMA 预留 **连续** 大块；在伙伴系统里以 `MIGRATE_CMA` 实现（见上表） |

**HFT：** 用户态 **`mmap` + hugepage** 也按 **页框** 粒度映射 — **TLB 条目数** ∝ 覆盖 VA / page size。内核 **`__get_free_pages(order)`** 要 **物理连续** — 长时间运行 **碎片化** 后 **大块 order 失败** 类似用户态 **hugetlb 池耗尽**。

> **HFT 补充：THP 不是免费的。**
> Transparent Huge Page 由 `khugepaged` 内核线程在后台**扫描并合并** 4KB 页成 2MB 页，
> 合并过程需要**暂时冻结进程页表并做迁移**——表现为**几百微秒到毫秒级**的意外停顿。
> 追求尾延迟稳定的系统通常：① **静态预分配 hugetlb 池**并在启动时用掉（避免运行时 compaction 抖动）；
> ② 或把 THP 设为 `madvise` 模式，只对明确标记的区域生效（`/sys/kernel/mm/transparent_hugepage/enabled`）。
> **"开启 THP 就一定更快"是错的**——它用平均延迟换尾延迟。

→ [06 Gorman Ch2 页框](../../../06-linux-mm/chapter-02-describing-physical-memory/notes/section-3-物理页框.md) · [Ch 15 用户页表映射](../../chapter-15-process-address-space/)


> ↔ [ULK Ch8 §2 页框管理](../../../16-linux-kernel-deep/chapter-08-memory-management/notes/section-2-页框管理.md)


<details>
<summary>自测题（点击展开）</summary>

**Q1.** 物理页大小为什么是 4KB？Huge Page 对 HFT 有什么好处？

<details><summary>答案</summary>

4KB 是历史折中：太小→页表占内存大（4GB/4KB=1M 项）；太大→内部碎片。Huge Page（x86: 2MB/1GB）减少 TLB miss：4KB 页需要 1000+ TLB 项覆盖 4GB，2MB 页只需 2000 项覆盖 4TB。HFT 用 Huge Page 映射行情数据/订单簿，TLB miss 下降 90%+。

</details>

**Q2.** struct page 是什么？为什么每个物理页都有一个？

<details><summary>答案</summary>

struct page 是内核管理物理页的元数据（flags/引用计数/映射计数/所属zone）。每个物理页一个，4GB 内存 = 1M 页 × 64 字节/page = 64MB page 数组。这是内核固定开销。page 结构体不包含页内数据，数据在物理地址对应的空间。

</details>

**Q3.** v6.6 里单次 `alloc_pages()` 最大能要多少？想要 16MB 物理连续内存该怎么办？

<details><summary>答案</summary>

上限是 **4MB**：`include/linux/mmzone.h:28` 里 `MAX_ORDER` 默认为 **10**（`!CONFIG_ARCH_FORCE_MAX_ORDER` 时），`MAX_ORDER_NR_PAGES = 1 << 10 = 1024` 页 × 4KB = 4MB。想要更大的**物理连续**内存，不能靠运行时的高阶分配（长时间运行后必然碎片化），正确路径是：① 启动时用 `hugetlb`/`memblock` 预留；② 用 **CMA** 预留区（运行时由驱动申请，不用时仍可被可移动页借用）；③ 重审需求——若只是要**虚拟连续**，`vmalloc()` 即可（见 12.6）。

</details>

**Q4.** 什么是迁移类型（migratetype）？它怎么解决碎片化？

<details><summary>答案</summary>

伙伴系统的每个 order 空闲链表又按 `migratetype` 细分（`free_list[MIGRATE_TYPES]`），把"可移动的页"（用户态匿名页、页缓存，`MIGRATE_MOVABLE`）与"不可移动的页"（内核数据，`MIGRATE_UNMOVABLE`）隔离存放，另有 RECLAIMABLE、HIGHATOMIC、CMA、ISOLATE。要的类型没页时按回落表偷（`page_alloc.c:1597`：UNMOVABLE→RECLAIMABLE→MOVABLE）。效果是让可移动页聚成大块连续区，需要连续物理内存时通过**迁移**腾出空间（CMA、内存热拔、THP 都依赖它）。这是"长时间运行后高阶分配失败"的结构性解药，而不是靠运气。

</details>

</details>
---
