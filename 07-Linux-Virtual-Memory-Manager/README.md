# Understanding the Linux Virtual Memory Manager — Mel Gorman

**文件夹 03** · 全书 **14 章 + 附录 A–M** · [返回总清单](../READING-LIST.md#3-understanding-the-linux-virtual-memory-manager--mel-gorman)

📋 **完整目录与 HFT 读/跳标注** → [OUTLINE.md](./OUTLINE.md)

---

## 主要章节（14 章）

| 章 | 笔记 |
|----|------|
| 1 简介 | [chapter-01-简介.md](./chapter-01-简介.md) |
| 2 描述物理内存 | [chapter-02-描述物理内存.md](./chapter-02-描述物理内存.md) |
| 3 页表管理 | [chapter-03-页表管理.md](./chapter-03-页表管理.md) |
| 4 进程地址空间 | [chapter-04-进程地址空间.md](./chapter-04-进程地址空间.md) |
| 5 启动内存分配器 | [chapter-05-启动内存分配器.md](./chapter-05-启动内存分配器.md) |
| 6 物理页分配 | [chapter-06-物理页分配.md](./chapter-06-物理页分配.md) |
| 7 非连续内存分配 | [chapter-07-非连续内存分配.md](./chapter-07-非连续内存分配.md) |
| 8 Slab 分配器 | [chapter-08-Slab分配器.md](./chapter-08-Slab分配器.md) |
| 9 高端内存管理 | [chapter-09-高端内存管理.md](./chapter-09-高端内存管理.md) |
| 10 页框回收 | [chapter-10-页框回收.md](./chapter-10-页框回收.md) |
| 11 交换管理 | [chapter-11-交换管理.md](./chapter-11-交换管理.md) |
| 12 共享内存虚拟文件系统 | [chapter-12-共享内存虚拟文件系统.md](./chapter-12-共享内存虚拟文件系统.md) |
| 13 内存耗尽管理 | [chapter-13-内存耗尽管理.md](./chapter-13-内存耗尽管理.md) |
| 14 结束语 | [chapter-14-结束语.md](./chapter-14-结束语.md) |

### HFT 延伸

| | 笔记 |
|---|------|
| 透明大页 THP | [note-透明大页THP.md](./note-透明大页THP.md) |

## 代码注释附录（A–M）

| | 笔记 |
|---|------|
| A 简介 | [appendix-A-简介.md](./appendix-A-简介.md) |
| B 描述物理内存 | [appendix-B-描述物理内存.md](./appendix-B-描述物理内存.md) |
| C 页表管理 | [appendix-C-页表管理.md](./appendix-C-页表管理.md) |
| D 进程地址空间 | [appendix-D-进程地址空间.md](./appendix-D-进程地址空间.md) |
| E 启动内存分配器 | [appendix-E-启动内存分配器.md](./appendix-E-启动内存分配器.md) |
| F 物理页分配 | [appendix-F-物理页分配.md](./appendix-F-物理页分配.md) |
| G 非连续内存分配 | [appendix-G-非连续内存分配.md](./appendix-G-非连续内存分配.md) |
| H Slab 分配器 | [appendix-H-Slab分配器.md](./appendix-H-Slab分配器.md) |
| I 高端内存管理 | [appendix-I-高端内存管理.md](./appendix-I-高端内存管理.md) |
| J 页框回收 | [appendix-J-页框回收.md](./appendix-J-页框回收.md) |
| K 交换管理 | [appendix-K-交换管理.md](./appendix-K-交换管理.md) |
| L 共享内存虚拟文件系统 | [appendix-L-共享内存虚拟文件系统.md](./appendix-L-共享内存虚拟文件系统.md) |
| M 内存耗尽管理 | [appendix-M-内存耗尽管理.md](./appendix-M-内存耗尽管理.md) |

---

## HFT 精读捷径

```
Ch 1（选读 · 源码阅读路线）→ Ch 2 → Ch 3 (+ THP) → Ch 8 → Ch 4 → Ch 10
```

**Ch 1** 不教 VM 公式，给 **`mm/` 入口文件**（`oom_kill.c` → `vmalloc.c` → `page_alloc.c` → VMA）— 见 [chapter-01-简介.md](./chapter-01-简介.md#4-阅读代码的策略-reading-the-code)。

**HFT 产出：** 订单簿/内存池布局、NUMA 绑内存、伪共享（配合 Hennessy Ch2）的理论依据。

## 交叉阅读

- 内核概述 → [02-LKD](../05-Linux-Kernel-Development/00_Book_3rd_Notes/)
- 程序员落地 → [01-CSAPP Ch6/Ch9](../01-CSAPP-3rd/)
- DPDK 大页 → [10-DPDK](../14-DPDK-Low-Latency-Network/)
