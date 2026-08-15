## ④ bio 结构 · `struct bio`

2.6 引入 — **轻量级**、表示 **in-flight 块 I/O**。

| 特性 | 说明 |
|------|------|
| **`bio_vec` 数组** | 一个 I/O 可含 **多段内存** |
| **分散-聚集（scatter-gather）** | 内存 **不连续** · 磁盘上 **连续** |
| **高端内存** | 支持 HIGHMEM 映射 |
| **可分割** | RAID 等把 **一个大 bio** 分到多盘 |

```
一次读 4KB 文件块：
  bio
    └── bio_vec[0] → 页 A 中 2KB
    └── bio_vec[1] → 页 B 中 2KB     （内存散，磁盘连续）
```

→ **Ch 12** 页 · **Ch 16** 页缓存提交 bio



<details>
<summary>自测题（点击展开）</summary>

**Q1.** bio_vec 的作用是什么？为什么需要 scatter-gather？

<details><summary>答案</summary>

bio_vec 描述一个物理段（page + offset + length）。一个 bio 包含多个 bio_vec，可以表示不连续物理内存的 IO。scatter-gather 让 DMA 一次传输多个物理段，减少 DMA 次数。NVMe 原生支持 PRP/SGL，一个命令传输 4MB 数据。这就是为什么 NVMe 比 SATA 快——不仅仅是带宽，更是 IO 效率。

</details>

</details>
---
