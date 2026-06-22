# Ch 9 虚拟内存 · Virtual Memory

> **CSAPP 3rd** · Bryant & O'Neill · **精读 🔴**（Part II）

> 本章定位：**每个进程的「大私有内存」怎么实现** — 页表、TLB、缺页、`mmap`、堆分配器。HFT 地基核心：**大页、mlock、NUMA、热路径零 malloc、零缺页**。

---

## 小节笔记

| 节 | 笔记 |
|----|------|
| 9.1–9.3 物理/虚拟寻址与页式缓存 | [notes/section-9.1-9.3-物理虚拟寻址与页式缓存.md](./notes/section-9.1-9.3-物理虚拟寻址与页式缓存.md) |
| 9.4–9.5 VM 作为管理与保护工具 | [notes/section-9.4-9.5-VM管理与保护.md](./notes/section-9.4-9.5-VM管理与保护.md) |
| 9.6–9.7 地址翻译、TLB 与 Linux 案例 | [notes/section-9.6-9.7-地址翻译与Linux案例.md](./notes/section-9.6-9.7-地址翻译与Linux案例.md) |
| 9.8 内存映射 mmap | [notes/section-9.8-内存映射mmap.md](./notes/section-9.8-内存映射mmap.md) |
| 9.9.1–9.9.6 malloc 基础与碎片 | [notes/section-9.9.1-9.9.6-malloc基础与碎片.md](./notes/section-9.9.1-9.9.6-malloc基础与碎片.md) |
| 9.9.7–9.9.14 分配器实现 | [notes/section-9.9.7-9.9.14-分配器实现.md](./notes/section-9.9.7-9.9.14-分配器实现.md) |
| 9.10 垃圾收集 | [notes/section-9.10-垃圾收集.md](./notes/section-9.10-垃圾收集.md) |
| 9.11–9.12 内存错误与小结 | [notes/section-9.11-9.12-内存错误与小结.md](./notes/section-9.11-9.12-内存错误与小结.md) |

---

## 大白话 · 本章一条线

> **虚拟地址是「门牌号」，MMU+页表翻译成物理 RAM；不够就缺页，从磁盘换页进来。**

```
进程看到连续 VA → 页表 → PA（DRAM）
miss → page fault → 内核分配/换入
mmap：把文件或匿名内存映射进 VA
malloc：堆上 bump / 空闲链表管理
```

**HFT 铁律：**

1. **tick 路径零缺页** — `mlock`/`MAP_LOCKED`、启动时 touch 全工作集
2. **大页 (hugepage/THP)** — 降 TLB miss（→ DPDK EAL、[06-Gorman THP](../../../06-Linux-Virtual-Memory-Manager/)）
3. **热路径零 malloc** — 池化、arena、Rust 栈上/预分配

---

## 本章 Checklist

- [ ] 说出 VM 三大功能：**缓存、管理、保护**
- [ ] 解释 **页命中 vs 缺页**；为何局部性仍关键
- [ ] 描述 **TLB** 作用；多级页表为何节省内存
- [ ] 会用 `mmap`/`munmap`；理解 `MAP_SHARED` vs `MAP_PRIVATE`
- [ ] 知道 `brk`/`sbrk` 与堆；`malloc` 底层碎片与合并
- [ ] 列举 9.11 常见 bug：UAF、泄漏、off-by-one
- [ ] 会查 `/proc/<pid>/maps`、`numastat`、hugepage 配置

---

## HFT 精读捷径

```
必读：9.3 缺页 · 9.6 TLB/大页 · 9.8 mmap · 9.11 错误 · 9.9 懂 malloc 即可
内核深入：9.7 + 06-Gorman · 02-SysPerf Ch7
分配器实现 9.9.6–9.9.14：课程/面试；生产用 tcmalloc/jemalloc/池
9.10 GC：懂概念；C++ 热路径不用
```

---

## 相关章节

- 上一章：[../chapter-08-exceptional-control-flow/](../chapter-08-exceptional-control-flow/)
- 下一章：[../chapter-10-系统级IO.md](../chapter-10-系统级IO.md)
- Cache：[../chapter-06-memory-hierarchy/](../chapter-06-memory-hierarchy/)
- 链接加载：[../chapter-07-linking/](../chapter-07-linking/)
- 内核 VM：[06-Gorman](../../../06-Linux-Virtual-Memory-Manager/)
- 全书目录：[OUTLINE.md](../OUTLINE.md)
