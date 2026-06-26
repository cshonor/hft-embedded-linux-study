# Ch 8 文件系统 · File Systems

> **BPF Performance Tools** · Brendan Gregg · **跳过 ⚪**

> 本章定位：**应用程序视角的逻辑 I/O** — 应用通常不直接碰磁盘，而是经 **VFS + 页缓存** 读写。文件系统用 **缓存、预读、写回、异步 I/O** 把物理盘延迟藏起来；BPF 工具测量的是 **应用在逻辑 I/O 上真实等待的时间**。  
> **HFT：** 热路径应 **无同步盘 I/O**（行情/下单走内存、网络、DPDK）；本章默认 **⚪ 跳过**，仅在 **日志风暴、`mmap` 数据文件、配置热读、共置机 page cache 争抢** 等 incident 时查阅 `opensnoop` / `cachestat` / `fileslower`。  
> **上一章：** [chapter-07-内存.md](../chapter-07-memory/) · **下一章：** [chapter-09-磁盘IO.md](../chapter-09-disk-io/)

---

## 小节笔记

| 节 | 笔记 |
|----|------|
| 1 本章要回答的问题 | [notes/section-1-本章要回答的问题.md](./notes/section-1-本章要回答的问题.md) |
| 2 文件系统基础 (Background) | [notes/section-2-文件系统基础.md](./notes/section-2-文件系统基础.md) |
| 3 传统文件系统工具 | [notes/section-3-传统文件系统工具.md](./notes/section-3-传统文件系统工具.md) |
| 4 VFS 与应用 I/O 分析 | [notes/section-4-VFS与应用IO分析.md](./notes/section-4-VFS与应用IO分析.md) |
| 5 缓存效能分析 | [notes/section-5-缓存效能分析.md](./notes/section-5-缓存效能分析.md) |
| 6 内存映射与文件生命周期 | [notes/section-6-内存映射与文件生命周期.md](./notes/section-6-内存映射与文件生命周期.md) |
| 7 具体文件系统：ext4 / XFS | [notes/section-7-具体文件系统ext4XFS.md](./notes/section-7-具体文件系统ext4XFS.md) |
| 8 工具选型速查 | [notes/section-8-工具选型速查.md](./notes/section-8-工具选型速查.md) |
| 9 与 Ch 7 / Ch 9 的衔接 | [notes/section-9-与Ch7Ch9的衔接.md](./notes/section-9-与Ch7Ch9的衔接.md) |

---

## 大白话

> 应用程序视角的逻辑 I/O

下面按原书小节展开；细节见 **小节笔记** 表。

---

## 本章 Checklist

- [ ] **默认 ⚪ 跳过**— 低延迟交易不应在热路径同步读盘；若 `opensnoop` 在 tick 路径上频繁出现，即 **架构 red flag**。
- [ ] **incident 三板斧：**`filetop`（谁在写）→ `fileslower`（是否同步慢 I/O）→ `cachestat`（是否内存/cache 问题）。
- [ ] **日志与配置**是 HFT 机上最常见的 FS 噪声 — `opensnoop` 查意外路径，`filelife` 查临时文件。
- [ ] **`mmap` 行情/历史数据**用 `mmapfiles`/`fmapfault` + [Ch 7 `faults`](./chapter-07-内存.md) — 冷启动 vs 稳态分开看。
- [ ] **`strace` 勿上生产热路径**— 用 BCC 聚合工具替代。

---

## 相关章节

- 上一章：[chapter-07-内存.md](../chapter-07-memory/)
- 下一章：[chapter-09-磁盘IO.md](../chapter-09-disk-io/)
- VFS 教学 OS：[01-30days-os day-18-dir](../09-system-low-level-hands-on/01-30days-os/day-18-dir/)
- SysPerf 文件系统：[chapter-08-file-systems](../../03-Systems-Performance-2nd/chapter-08-file-systems/)（若存在）
- 方法论：[chapter-03-性能分析.md](../chapter-03-performance-analysis/)
