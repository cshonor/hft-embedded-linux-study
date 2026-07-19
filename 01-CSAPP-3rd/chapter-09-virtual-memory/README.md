# Ch 9 虚拟内存 · Virtual Memory

> **CSAPP 3rd** · Bryant & O'Neill · **精读 🔴**（Part II）

> 本章定位：**每个进程的「大私有内存」怎么实现** — 页表、TLB、缺页、`mmap`、堆分配器。HFT 地基核心：**大页、mlock、NUMA、热路径零 malloc、零缺页**。

---

## 小节笔记

| 节 | 笔记 |
|----|------|
| 9.1 物理和虚拟寻址·名字辨析 | [notes/section-9.1-物理和虚拟寻址·名字辨析.md](./notes/section-9.1-物理和虚拟寻址·名字辨析.md) |
| 9.2 地址空间 | [notes/section-9.2-地址空间.md](./notes/section-9.2-地址空间.md) |
| 9.3 虚拟内存作为缓存工具 | [notes/section-9.3-虚拟内存作为缓存工具.md](./notes/section-9.3-虚拟内存作为缓存工具.md) |
| 9.4 虚拟内存作为内存管理工具 | [notes/section-9.4-虚拟内存作为内存管理工具.md](./notes/section-9.4-虚拟内存作为内存管理工具.md) |
| 9.5 虚拟内存作为保护工具 | [notes/section-9.5-虚拟内存作为保护工具.md](./notes/section-9.5-虚拟内存作为保护工具.md) |
| 9.6 地址翻译 | [notes/section-9.6-地址翻译.md](./notes/section-9.6-地址翻译.md) |
| 9.7 IntelCorei7-Linux案例 | [notes/section-9.7-IntelCorei7-Linux案例.md](./notes/section-9.7-IntelCorei7-Linux案例.md) |
| 9.8 内存映射mmap | [notes/section-9.8-内存映射mmap.md](./notes/section-9.8-内存映射mmap.md) |
| 9.9.1 malloc和free | [notes/section-9.9.1-malloc和free.md](./notes/section-9.9.1-malloc和free.md) |
| 9.9.2 为何动态分配 | [notes/section-9.9.2-为何动态分配.md](./notes/section-9.9.2-为何动态分配.md) |
| 9.9.3 分配器目标 | [notes/section-9.9.3-分配器目标.md](./notes/section-9.9.3-分配器目标.md) |
| 9.9.4 碎片 | [notes/section-9.9.4-碎片.md](./notes/section-9.9.4-碎片.md) |
| 9.9.5-9.9.6 实现问题与隐式空闲链表 | [notes/section-9.9.5-9.9.6-实现问题与隐式空闲链表.md](./notes/section-9.9.5-9.9.6-实现问题与隐式空闲链表.md) |
| 9.9.7-9.9.9 放置、分割、扩展堆 | [notes/section-9.9.7-9.9.9-放置、分割、扩展堆.md](./notes/section-9.9.7-9.9.9-放置、分割、扩展堆.md) |
| 9.9.10-9.9.11 合并与边界标记 | [notes/section-9.9.10-9.9.11-合并与边界标记.md](./notes/section-9.9.10-9.9.11-合并与边界标记.md) |
| 9.9.12 简单分配器综合 | [notes/section-9.9.12-简单分配器综合.md](./notes/section-9.9.12-简单分配器综合.md) |
| 9.9.13 显式空闲链表 | [notes/section-9.9.13-显式空闲链表.md](./notes/section-9.9.13-显式空闲链表.md) |
| 9.9.14 分离空闲链表 | [notes/section-9.9.14-分离空闲链表.md](./notes/section-9.9.14-分离空闲链表.md) |
| 9.10 垃圾收集 | [notes/section-9.10-垃圾收集.md](./notes/section-9.10-垃圾收集.md) |
| 9.11 C程序常见内存错误 | [notes/section-9.11-C程序常见内存错误.md](./notes/section-9.11-C程序常见内存错误.md) |
| 9.12 小结 | [notes/section-9.12-小结.md](./notes/section-9.12-小结.md) |


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
2. **大页 (hugepage/THP)** — 降 TLB miss（→ DPDK EAL、[06-Linux-Virtual-Memory-Manager THP](../../../06-Linux-Virtual-Memory-Manager/)）
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
内核深入：9.7 + 07-Gorman · 03-SysPerf Ch7
分配器实现 9.9.6–9.9.14：课程/面试；生产用 tcmalloc/jemalloc/池
9.10 GC：懂概念；C++ 热路径不用
```

---

## 相关章节

- 上一章：[../chapter-08-exceptional-control-flow/](../chapter-08-exceptional-control-flow/)
- 下一章：[../chapter-10-system-io/](../chapter-10-system-io/)
- Cache：[../chapter-06-memory-hierarchy/](../chapter-06-memory-hierarchy/)
- 链接加载：[../chapter-07-linking/](../chapter-07-linking/)
- 内核 VM：[06-Linux-Virtual-Memory-Manager](../../../06-Linux-Virtual-Memory-Manager/)
- 全书目录：[OUTLINE.md](../OUTLINE.md)
