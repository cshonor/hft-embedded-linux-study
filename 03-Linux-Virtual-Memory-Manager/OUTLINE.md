# Gorman — 全书目录（14 章 + 附录 A–M）

> **Understanding the Linux Virtual Memory Manager** · Mel Gorman

| 标签 | HFT 读法 |
|------|----------|
| 🔴 | 精读 |
| 🟡 | 选读 |
| ⚪ | 跳过 |

## 主要章节

| 章 | 英文 | 笔记 | HFT |
|----|------|------|-----|
| 1 | Introduction | [chapter-01](./chapter-01-简介.md) | 🟡 |
| 2 | Describing Physical Memory | [chapter-02](./chapter-02-描述物理内存.md) | 🔴 |
| 3 | Page Table Management | [chapter-03](./chapter-03-页表管理.md) | 🔴 |
| 4 | Process Address Space | [chapter-04](./chapter-04-进程地址空间.md) | 🟡 |
| 5 | Boot Memory Allocator | [chapter-05](./chapter-05-启动内存分配器.md) | ⚪ |
| 6 | Physical Page Allocation | [chapter-06](./chapter-06-物理页分配.md) | 🟡 |
| 7 | Noncontiguous Memory Allocation | [chapter-07](./chapter-07-非连续内存分配.md) | ⚪ |
| 8 | Slab Allocator | [chapter-08](./chapter-08-Slab分配器.md) | 🔴 |
| 9 | High Memory Management | [chapter-09](./chapter-09-高端内存管理.md) | ⚪ |
| 10 | Page Frame Reclamation | [chapter-10](./chapter-10-页框回收.md) | 🟡 |
| 11 | Swap Management | [chapter-11](./chapter-11-交换管理.md) | ⚪ |
| 12 | Shared Memory Virtual Filesystem | [chapter-12](./chapter-12-共享内存虚拟文件系统.md) | 🟡 |
| 13 | Out of Memory Management | [chapter-13](./chapter-13-内存耗尽管理.md) | ⚪ |
| 14 | The Final Word | [chapter-14](./chapter-14-结束语.md) | ⚪ |

### HFT 延伸（非原书专章）

| 主题 | 笔记 | HFT |
|------|------|-----|
| 透明大页 THP | [note-透明大页THP](./note-透明大页THP.md) | 🔴 |

## 代码注释附录（Code Commentary）

| | 英文 | 笔记 | HFT |
|---|------|------|-----|
| A | Introduction | [appendix-A](./appendix-A-简介.md) | ⚪ |
| B | Describing Physical Memory | [appendix-B](./appendix-B-描述物理内存.md) | 🟡 |
| C | Page Table Management | [appendix-C](./appendix-C-页表管理.md) | 🟡 |
| D | Process Address Space | [appendix-D](./appendix-D-进程地址空间.md) | 🟡 |
| E | Boot Memory Allocator | [appendix-E](./appendix-E-启动内存分配器.md) | ⚪ |
| F | Physical Page Allocation | [appendix-F](./appendix-F-物理页分配.md) | ⚪ |
| G | Noncontiguous Memory Allocation | [appendix-G](./appendix-G-非连续内存分配.md) | ⚪ |
| H | Slab Allocator | [appendix-H](./appendix-H-Slab分配器.md) | 🟡 |
| I | High Memory Management | [appendix-I](./appendix-I-高端内存管理.md) | ⚪ |
| J | Page Frame Reclamation | [appendix-J](./appendix-J-页框回收.md) | ⚪ |
| K | Swap Management | [appendix-K](./appendix-K-交换管理.md) | ⚪ |
| L | Shared Memory Virtual Filesystem | [appendix-L](./appendix-L-共享内存虚拟文件系统.md) | ⚪ |
| M | Out of Memory Management | [appendix-M](./appendix-M-内存耗尽管理.md) | ⚪ |

> 前言 / 参考文献 / 索引：不单独建笔记文件。

---

## HFT 精读顺序

```
Ch 2  物理内存 / Zones / NUMA
Ch 3  页表 / TLB / 大页  +  note-透明大页THP
Ch 8  Slab / Slub
Ch 4  进程地址空间（mmap、缺页）
Ch 10 页框回收（避免运行时抖动）
附录 B/C/H（有余力时代码走读）
```

→ 程序员落地 → [08-CSAPP-3rd Ch9](../08-CSAPP-3rd/chapter-09-虚拟内存.md)  
→ 内核概述 → [02-LKD Ch11](../02-Linux-Kernel-Development-3rd/chapter-12-内存管理.md)

完整路线 → [HFT-READING-ROADMAP.md](../HFT-READING-ROADMAP.md)
