# 06.5-modern-mm

> 定位：**现代Linux内核（5.x / 6.x）内存管理子系统参考资料**
> 前置：`06-linux-mm`（Mel Gorman《Understanding the Linux Virtual Memory Manager》，基于2.4/2.6）
> 本目录存放现代内存管理资料，弥补 Mel Gorman 书的大量过时实现；
> 学习完本目录材料之后，再回到 `06-linux-mm` 做源码阅读与实操实验。

## 资料来源

1. 笨叔《奔跑吧 Linux 内核》卷1 Ch3-6（内存管理预备知识 / 物理内存与虚拟内存 / 高级主题 / 实战案例）
2. LWN.net 深度专题文章，专门修正 Mel Gorman 书的过时算法、数据结构
3. Bootlin 公开培训讲义（内存管理子系统）

## 章节索引

| 章 | 主题 | 来源 | 目录 |
|----|------|------|------|
| 01 | 物理内存管理与 Memblock | Bootlin + LWN | [chapter-01](chapter-01-physical-memory-memblock/) |
| 02 | SLAB/SLUB 分配器 | Bootlin + LWN | [chapter-02](chapter-02-slab-slub-allocator/) |
| 03 | vmalloc 与内核内存分配 | Bootlin + 笨叔 | [chapter-03](chapter-03-vmalloc-kernel-alloc/) |
| 04 | 页表与 TLB | Bootlin + LWN | [chapter-04](chapter-04-page-table-tlb/) |
| 05 | 虚拟地址空间与 Maple Tree | 笨叔 + LWN | [chapter-05](chapter-05-vm-address-space-maple-tree/) |
| 06 | 页缓存与 Folio | 笨叔 + Bootlin + LWN | [chapter-06](chapter-06-page-cache-folio/) |
| 07 | 页回收与 LRU/MGLRU | 笨叔 + Bootlin + LWN | [chapter-07](chapter-07-page-reclaim-mglru/) |
| 08 | OOM/PSI/zswap | Bootlin + LWN | [chapter-08](chapter-08-oom-psi-zswap/) |
| 09 | Memory Cgroup 与监控 | Bootlin | [chapter-09](chapter-09-memcg-monitoring/) |
| 10 | DAMON | Bootlin + LWN | [chapter-10](chapter-10-damon/) |

## 学习流转顺序

1. `06-linux-mm`：理解内存管理需要解决什么问题，**不要照搬旧版代码实现**
2. `06.5-modern-mm`：学习5.x~6.x真正的现代MM实现
3. `06-linux-mm`：阅读树莓派内核源码 `mm/` 目录、调试实验

### ⚠️ 关键警告

Mel Gorman 的书基于Linux2.4/2.6。**bootmem已删除→memblock、SLAB已移除→SLUB、highmem在ARM64不存在、LRU→MGLRU、page→folio。禁止直接对照源码查找旧API。本目录全部材料用来补齐时代差异。**

### 与 06-linux-mm 的衔接映射

| Mel Gorman 旧主题 | 本书旧章 | 现代 6.x 替代 | 本目录章节 |
|-------------------|----------|--------------|-----------|
| bootmem | Ch2 | memblock | ch01 |
| SLAB | Ch3 | SLUB | ch02 |
| 非连续内存 | Ch4 | vmalloc（不变） | ch03 |
| 页表 | Ch4 | 5 级页表 | ch04 |
| VMA 红黑树 | Ch4 | maple tree | ch05 |
| page cache | Ch8 | folio | ch06 |
| LRU 回收 | Ch9/10 | MGLRU | ch07 |
| OOM | — | OOM + PSI | ch08 |
| — | — | memcg + 监控 | ch09 |
| — | — | DAMON | ch10 |

## 参考索引文件

- [ref-mm-outdated-mapping.md](./ref-mm-outdated-mapping.md) — Mel Gorman 过时评估 + 笨叔卷1映射 + LWN MM精选
