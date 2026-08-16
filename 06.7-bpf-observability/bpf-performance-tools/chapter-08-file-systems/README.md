# Ch 8 文件系统 · File Systems

> **BPF Performance Tools** · Brendan Gregg · **选读 🟡**

> 本章定位：**应用程序视角的逻辑 I/O** — 应用通常不直接碰磁盘，而是经 **VFS + 页缓存** 读写。文件系统用 **缓存、预读、写回** 把物理盘延迟藏起来；BPF 工具测量的是 **应用在逻辑 I/O 上真实等待的时间**（fileslower/xfsslower 的高延迟可直接定罪）。
> **HFT：** 热路径应 **无同步盘 I/O**；日志风暴、`mmap` 数据文件、配置热读、共置机 page cache 争抢等 incident 时用 `opensnoop` / `cachestat` / `fileslower` 下钻。
> **上一章：** [chapter-07-内存](../chapter-07-memory/) · **下一章：** [chapter-09-磁盘IO](../chapter-09-disk-io/)

---

## 小节笔记（对齐原书 8.1–8.6 真实目录）

| 节 | 原书标题 | 笔记 |
|----|----------|------|
| 8.1 | 背景知识（文件系统基础 / BPF 能力 / 十步策略） | [notes/section-1-背景知识.md](./notes/section-1-背景知识.md) |
| 8.2 | 传统工具（df / mount / strace / perf / fatrace） | [notes/section-2-传统工具.md](./notes/section-2-传统工具.md) |
| 8.3.1–6 | BPF 工具 · 系统调用层（opensnoop/statsnoop/syncsnoop/mmapfiles/scread/fmapfault） | [notes/section-3-BPF工具-系统调用跟踪.md](./notes/section-3-BPF工具-系统调用跟踪.md) |
| 8.3.7–11 | BPF 工具 · VFS 统计（filelife/vfsstat/vfscount/vfssize/fsrwstat） | [notes/section-4-BPF工具-VFS统计.md](./notes/section-4-BPF工具-VFS统计.md) |
| 8.3.12–15 | BPF 工具 · 慢操作与 Top（fileslower/filetop/writesync/filetype） | [notes/section-5-BPF工具-慢操作与文件top.md](./notes/section-5-BPF工具-慢操作与文件top.md) |
| 8.3.16–17 | BPF 工具 · 页缓存与写回（cachestat/writeback） | [notes/section-6-BPF工具-页缓存与写回.md](./notes/section-6-BPF工具-页缓存与写回.md) |
| 8.3.18–20 | BPF 工具 · 目录/inode 缓存（dcstat/dcsnoop/mountsnoop） | [notes/section-7-BPF工具-目录与inode缓存.md](./notes/section-7-BPF工具-目录与inode缓存.md) |
| 8.3.21–27 | BPF 工具 · 文件系统特定（xfsslower/xfsdist/ext4dist/bufgrow/readahead/其他） | [notes/section-8-BPF工具-文件系统特定.md](./notes/section-8-BPF工具-文件系统特定.md) |
| 8.4 | BPF 单行程序（BCC / bpftrace） | [notes/section-9-BPF单行程序.md](./notes/section-9-BPF单行程序.md) |
| 8.5 | 可选练习（7 题，第 7 题作者未解决） | [notes/section-10-可选练习.md](./notes/section-10-可选练习.md) |
| 8.6 | 小结 | [notes/section-11-小结.md](./notes/section-11-小结.md) |

---

## 大白话：三板斧 + 两条线

**三板斧（incident 时按序用）**：

1. **filetop** — 谁在读写什么文件（I/O 热点定性，文件版 top）
2. **fileslower** — 哪些**同步**读写 >10ms（这些是应用真正在等的；磁盘层工具做不到这一点）
3. **cachestat** — 页缓存命中率多少（90%→100% 的收益远超 10%，全命中=纯内存跑）

**两条线**：

- **延迟线**：fileslower（VFS 同步慢操作）→ xfsslower/ext4dist（文件系统层逐事件/分布）→ 第 9 章块层。越靠近应用，延迟证据越硬。
- **缓存线**：cachestat（页缓存）→ dcstat/icstat（元数据缓存）→ readahead（预读是否浪费）→ writeback（写回风暴对时间轴）。

**分层口诀**：系统调用层（opensnoop 家族，低频安全）→ VFS 层（vfsstat 家族，高频有开销，临时用）→ 文件系统层（dist/slower，贴近应用）→ 缓存（只此一家能测）。

---

## 本章 Checklist（HFT）

- [ ] **热路径无同步盘 I/O** — `opensnoop` 在 tick 路径频繁出现即架构 red flag；`writesync` 审计是否有误用 O_SYNC 的持久化写。
- [ ] **incident 三板斧**：`filetop`（谁）→ `fileslower`（多慢）→ `cachestat`（缓存兜底了吗）。
- [ ] **日志与配置**是交易机最常见的 FS 噪声 — `opensnoop` 查意外路径、`filelife` 查临时文件、`syncsnoop` 对延迟尖峰时间轴。
- [ ] **mmap 行情/历史数据**用 `mmapfiles`/`fmapfault` 配合 Ch7 `faults`，冷启动 vs 稳态分开看。
- [ ] **开盘前预加载验证**：`cachestat` 确认关键数据集 HITRATIO=100%（活跃数据集 ≤ 内存是容量规划第一问）。
- [ ] **strace/fatrace 勿上生产**（ptrace 掉到 1%、fanotify 67% CPU）；BCC 同类工具 1.1%。
- [ ] **SSD 数据盘**：用 `readahead` 验证预读页利用率，及时调小 read_ahead_kb。

---

## 相关章节

- 上一章：[chapter-07-内存](../chapter-07-memory/)（页缓存即内存：CACHED_MB / bufgrow 解释 free）
- 下一章：[chapter-09-磁盘IO](../chapter-09-disk-io/)（物理 I/O：biolatency/biosnoop；readahead 的 SSD 案例续讲）
- VFS 教学 OS：thirty-days-os day-18-dir
- SysPerf 文件系统：[chapter-08-file-systems](../../../06.6-systems-performance/chapter-08-file-systems/)（若存在）
- 方法论：[chapter-03-性能分析](../chapter-03-performance-analysis/)
