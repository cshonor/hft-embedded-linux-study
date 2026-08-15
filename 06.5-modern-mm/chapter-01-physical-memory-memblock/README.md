# Chapter 01: 物理内存管理与 Memblock

> 来源：Bootlin（物理内存管理）+ LWN（memblock）
> 对标：Mel Gorman Ch2（bootmem → memblock）

## 小节索引

| # | 文件 | 主题 |
|---|------|------|
| 1 | [physical-memory-management](notes/01-physical-memory-management.md) | Bootlin：物理内存布局、memblock API、reserved 区域 |
| 2 | [memblock](notes/02-memblock.md) | LWN：memblock 演进、bootmem 替代、早期分配流程 |

## HFT 关联

- **memblock reserved**：HFT 进程可通过 `memblock_reserve` 在启动早期保留大页连续物理内存，避免运行时分配延迟
- **NUMA 亲和**：物理内存分布直接影响 NIC DMA 与 CPU cache 的距离，影响收包延迟
- **大页预留**：启动时预留 hugepage 比运行时分配更可靠，避免碎片化

## 交叉引用

- `06-linux-mm/`：Mel Gorman 书中 bootmem 章节（已过时，本目录为现代替代）
- `05.5-modern-kernel/chapter-01-kernel-architecture/`：内核启动流程中的内存初始化
