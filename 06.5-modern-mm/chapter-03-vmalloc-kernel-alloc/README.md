# Chapter 03: vmalloc 与内核内存分配

> 来源：Bootlin（vmalloc）+ 笨叔卷1（内存分配预备知识）
> 对标：Mel Gorman Ch4

## 小节索引

| # | 文件 | 主题 |
|---|------|------|
| 1 | [vmalloc](notes/01-vmalloc.md) | Bootlin：vmalloc 实现、页表映射、vmalloc_to_page |
| 2 | [memory-alloc-ben-shu](notes/02-memory-alloc-ben-shu.md) | 笨叔：kmalloc/vmalloc/alloc_pages 对比、GFP 标志、NUMA 策略 |

## HFT 关联

- **kmalloc vs vmalloc**：kmalloc 物理连续（DMA 友好），vmalloc 仅虚拟连续（大块非连续）；HFT 网络缓冲区必须用 kmalloc
- **GFP 标志**：HFT 内核模块应避免 `GFP_KERNEL`（可能睡眠），中断上下文必须用 `GFP_ATOMIC`
- **alloc_pages 大页**：`alloc_pages(GFP_KERNEL, order)` 分配连续页，order 越大碎片风险越高

## 交叉引用

- `06.5-modern-mm/chapter-02-slab-slub-allocator/`：SLUB kmalloc 实现
- `06.5-modern-mm/chapter-04-page-table-tlb/`：vmalloc 的页表映射
