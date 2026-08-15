# Chapter 04: 页表与 TLB

> 来源：Bootlin（页表/TLB 管理）+ LWN（5 级页表）
> 对标：Mel Gorman Ch4（4 级页表 → 5 级）

## 小节索引

| # | 文件 | 主题 |
|---|------|------|
| 1 | [page-table-tlb](notes/01-page-table-tlb.md) | Bootlin：4/5 级页表、TLB flush、hugepage 映射 |
| 2 | [five-level-page-table](notes/02-five-level-page-table.md) | LWN：5 级页表引入（LA57）、P4D 层、56 位地址空间 |

## HFT 关联

- **TLB miss 代价**：一次 TLB miss 约 100-300 cycles（~50-100ns），HFT 热路径必须用大页减少 TLB miss
- **TLB shootdown**：多核 TLB flush 通过 IPI 传播，导致微秒级延迟抖动；HFT 应避免在热路径修改页表
- **hugepage**：2MB 大页将 TLB 覆盖范围扩大 512 倍，显著减少 miss
- **LA57**：5 级页表增加一层查表，非大页场景下 TLB miss 代价更高

## 交叉引用

- `06.5-modern-mm/chapter-03-vmalloc-kernel-alloc/`：vmalloc 的页表映射
- `07-arm-architecture/`：ARM64 页表格式（PGD/PUD/PMD/PTE）
