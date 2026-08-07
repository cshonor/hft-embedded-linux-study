# 09.5-modern-mm-book

> 定位：**现代Linux内核（5.x / 6.x）内存管理子系统参考资料**
> 前置：`09-linux-mm`（Mel Gorman《Understanding the Linux Virtual Memory Manager》，基于2.4/2.6）
> 本目录存放现代内存管理资料，弥补 Mel Gorman 书的大量过时实现；
> 学习完本目录材料之后，再回到 `09-linux-mm` 做源码阅读与实操实验。

## 资料来源

1. 笨叔《奔跑吧 Linux 内核》卷1 Ch3-6（内存管理预备知识 / 物理内存与虚拟内存 / 高级主题 / 实战案例）
2. LWN.net 深度专题文章，专门修正 Mel Gorman 书的过时算法、数据结构
3. Bootlin 公开培训讲义（内存管理子系统）

## 内部子目录

- `book-ben-shu-notes/`  笨叔卷1读书笔记（内存分配 / 地址空间 / 页缓存 / 页回收）
- `lwn-articles-summary/`  LWN文章摘要（SLUB / folio / maple tree / MGLRU / memblock / 5级页表 / PSI / DAMON）
- `bootlin-material/`  Bootlin MM 训练讲义要点 + 实验操作清单

## 学习流转顺序

1. `09-linux-mm`：理解内存管理需要解决什么问题，**不要照搬旧版代码实现**
2. `09.5-modern-mm-book`：学习5.x~6.x真正的现代MM实现
3. `09-linux-mm`：阅读树莓派内核源码 `mm/` 目录、调试实验

### ⚠️ 关键警告

Mel Gorman 的书基于Linux2.4/2.6。**bootmem已删除→memblock、SLAB已移除→SLUB、highmem在ARM64不存在、LRU→MGLRU、page→folio。禁止直接对照源码查找旧API。本目录全部材料用来补齐时代差异。**

## 参考索引文件

- [ref-mm-outdated-mapping.md](./ref-mm-outdated-mapping.md) — Mel Gorman 过时评估 + 笨叔卷1映射 + LWN MM精选
