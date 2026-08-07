# LWN 文章摘要 — 内存管理

> 对标 Mel Gorman《Understanding the Linux VM Manager》过时章节的 LWN.net 深度文章摘要。
> 每篇文章按：原文链接 + 核心观点 + 与旧书差异 + 关键代码变更 整理。

## 索引

### Slab / SLUB
- [ ] SLUB 分配器: https://lwn.net/Articles/229096/
- [ ] SLUB vs SLAB 性能: https://lwn.net/Articles/229160/
- [ ] SLAB 终被移除 (6.1+): https://lwn.net/Articles/949862/

### Folio / page API
- [ ] Folio 提案: https://lwn.net/Articles/849438/
- [ ] Folio 深入: https://lwn.net/Articles/862108/
- [ ] Folio 合入状态: https://lwn.net/Articles/893852/

### 地址空间
- [ ] Maple tree (6.1+): https://lwn.net/Articles/845507/

### 页回收 / MGLRU
- [ ] LRU 基础: https://lwn.net/Articles/845171/
- [ ] MGLRU 简介: https://lwn.net/Articles/856831/
- [ ] MGLRU 合入 6.1: https://lwn.net/Articles/913685/

### 其他现代内存主题
- [ ] memblock (取代 bootmem): https://lwn.net/Articles/449283/
- [ ] 5 级页表: https://lwn.net/Articles/717293/
- [ ] PSI 压力信息: https://lwn.net/Articles/759781/
- [ ] zswap: https://lwn.net/Articles/537422/
- [ ] DAMON: https://lwn.net/Articles/812704/

> 完整映射表见 [../ref-mm-outdated-mapping.md](../ref-mm-outdated-mapping.md)
