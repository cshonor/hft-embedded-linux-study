# LWN 文章摘要 — 内存管理

> 对标 Mel Gorman《Understanding the Linux VM Manager》过时章节的 LWN.net 深度文章摘要。
> 每篇文章按：原文链接 + 核心观点 + 与旧书差异 + 关键代码变更 + HFT 关联 + 自测题 整理。

## 索引

### Slab / SLUB
- [x] [SLUB 分配器](01-slub-allocator.md) — unqueued slab, per-CPU freelist, 快路径无锁
- [x] [SLUB vs SLAB 性能](02-slub-vs-slab.md) — 多线程 +15-20%, NUMA +30%
- [x] [SLAB 终被移除 (6.1+)](03-slab-removal.md) — 15 年后 SLAB 代码删除

### Folio / page API
- [x] [Folio 提案](04-folio-proposal.md) — 解决 page compound 歧义
- [x] [Folio 深入](05-folio-deep-dive.md) — folio API 设计细节 + large folio
- [x] [Folio 合入状态](06-folio-merge-status.md) — 5.16~6.3+ 渐进式迁移

### 地址空间
- [x] [Maple tree (6.1+)](07-maple-tree.md) — RCU-safe VMA 查找, 替换红黑树

### 页回收 / MGLRU
- [x] [LRU 基础](08-lru-basics.md) — active/inactive 双链表, kswapd 流程
- [x] [MGLRU 简介](09-mglru-intro.md) — 多代分级, 减少 90% 页扫描
- [x] [MGLRU 合入 6.1](10-mglru-merge.md) — Google 开发, Chrome OS 验证

### 其他现代内存主题
- [x] [memblock (取代 bootmem)](11-memblock.md) — 区域数组替代位图, 3.9+
- [x] [5 级页表](12-five-level-page-table.md) — 48→57 bit VA, 256TB→128PB
- [x] [PSI 压力信息](13-psi-pressure.md) — CPU/内存/I/O 精细压力指标, 4.20+
- [x] [zswap](14-zswap.md) — 内存中压缩 swap 缓存, 3.11+
- [x] [DAMON](15-damon.md) — 低开销数据访问监控, 5.15+

> 完整映射表见 [../ref-mm-outdated-mapping.md](../ref-mm-outdated-mapping.md)
