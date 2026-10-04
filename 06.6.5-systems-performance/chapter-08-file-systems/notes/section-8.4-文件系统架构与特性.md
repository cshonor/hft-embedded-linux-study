## 8.4 文件系统架构与特性

> 章节导航：[本章导读](../README.md) · 下一篇 [8.5 分析方法论](./section-8.5-分析方法论.md)

**本节讲什么**：VFS 抽象层的分析价值（统一观测点）、三大缓存的分工与联动、journal 模式对写路径的影响、主流 FS 的 HFT 选型。

### 要点

| # | 要点 | 一句话 |
|---|------|--------|
| 1 | VFS 是 BPF 追踪的**统一观测点** | 不管底层什么 FS |
| 2 | page cache 同时缓存**读与写** | 写是攒着异步刷的 |
| 3 | journal 三种模式 = **一致性/性能旋钮** | ordered 是默认平衡 |
| 4 | `free` 低 ≠ 内存不够 | cache 可回收（直到 direct reclaim） |
| 5 | HFT 选型：ext4/xfs + noatime + 物理分盘 | — |

---

### 一、VFS（虚拟文件系统）

```
App: read() / write() / open()
         ↓
    VFS: vfs_read() / vfs_write()  ← BPF 追踪统一点
         ↓
    ext4 / xfs / zfs / tmpfs / procfs...
         ↓
    page cache（文件 FS）→ block layer → disk
```

**性能分析价值**：可在**应用 syscall**、**VFS**、**具体 FS（ext4_xfs_*）** 各层量延迟——三层对比本身就是定位法：syscall 慢而 VFS 快 = 应用层开销；VFS 慢而 FS 快 = 缓存/锁；FS 层慢 = 真 I/O（[8.5 延迟分析](./section-8.5-分析方法论.md)）。

VFS 的抽象也让 **tmpfs**（纯内存 FS）、**procfs** 走同一接口——`read()` 到 tmpfs 就是内存拷贝，没有块层。

### 二、Linux 缓存三层

| 缓存 | 存什么 | 工具 | 失效代价 |
|------|--------|------|---------|
| **Page Cache** | 文件**内容**页（读缓存 + 写缓冲） | `free`、`cachestat` | 一次盘 I/O |
| **Dentry Cache** | 路径名 → inode 映射（目录项） | `sar -v` | 再走一遍路径解析 |
| **Inode Cache** | inode 结构/属性 | `sar -v`、`slabtop` | 从盘上读 inode |

三层联动：`open("/data/foo/bar")` 要走 dentry 查 `/data`→`foo`→`bar`（每级一次 lookup），拿到 inode 后 read 才查 page cache——**长路径 + 冷 dentry = 元数据 I/O 风暴**（每次 lookup 都可能是一次盘读）。容器/大量短生命周期文件场景 dentry/inode slab 占用可观（[ch7 内存](../../chapter-07-memory/)、slab 机制见 [06-linux-mm ch08](../../../06-linux-mm/chapter-08-slab-allocator/)）。

**⭐ page cache 的双重身份**：

```
读路径：miss → 发起块 I/O → 填页 → 返回（下次命中免 I/O）
写路径：write() → 只写 page cache（标记脏）→ 立即返回！
        → writeback 线程异步刷盘（脏页超阈值/超时）
        → fsync() 强制把本文件的脏页刷完 + 等
```

**写快的代价**：write() 返回 µs 级是「记在账上」，真落盘要等 writeback/fsync——**崩溃时未刷脏页丢失**（journal 只保元数据一致性，data=ordered 下数据页顺序有保证但内容未必最新）。延迟敏感的应用必须明白这个账期：日志「写成功」的语义分 `write()` 返回（内存）和 `fsync()` 返回（盘上）两档。

### 三、高级特性

| 特性 | 作用 | 性能 |
|------|------|------|
| **Extents** | 连续块区间分配（代替间接块）——减碎片、减元数据 | 顺序大文件友好 |
| **Journaling** | 崩溃一致性——元数据变更先写日志 | 增写放大；commit 有节奏 |
| **COW** | 写时复制——快照/克隆/校验的基础 | btrfs/ZFS；写路径变复杂（重定向） |
| **Delayed allocation** | 攒够再分配块（XFS/ext4 delalloc） | 减碎片，但 fsync 更重 |

**ext4 journal 三模式**（一致性/性能旋钮）：

| 模式 | journal 内容 | 崩溃后 | 性能 |
|------|-------------|--------|------|
| `data=journal` | 元数据 + **数据** | 数据不丢 | 最慢（全双写） |
| `data=ordered`（默认） | 元数据；数据先于元数据落盘 | 数据旧但一致 | 平衡 |
| `data=writeback` | 仅元数据 | **可能见到新旧混合数据** | 最快，有合规风险 |

journal 是 I/O 来源：ext4 的 `jbd2` 线程周期性 commit（默认 5s）——biostacks 里的常客（[ch9](../../chapter-09-disks/notes/section-9.5-分析方法论.md)）；日志盘与数据盘分离（ext4 可把 journal 放独立设备）是老牌优化。

### 四、常见文件系统选型

| FS | 特点 | HFT 场景 |
|----|------|----------|
| **ext4** | 默认、成熟、工具链全 | 系统盘、通用 |
| **XFS** | 大文件、并行分配组（allocation groups）、延迟分配 | 大容量日志/数据归档、高并发写 |
| **ZFS** | ARC 自适应缓存、存储池、快照/校验 | 非 tick 路径；recordsize 匹配 I/O 大小 |
| **btrfs** | COW、快照、压缩 | 备份、开发环境（COW 写放大不适合日志热路径） |
| **tmpfs** | 纯内存 | replay 缓冲、IPC 中转——盘 I/O 直接消失 |

**XFS 的 AG（allocation group）**：把盘切成多个独立分配区——多线程同时写大文件时各用各的 AG **分配锁不互斥**（与 blk-mq、SLUB per-CPU 同一哲学：共享变分区）。

### 五、HFT 实践

- 系统盘 ext4/xfs + **`noatime`**（每次读文件不再更新 atime——省一次元数据写）。
- NVMe 日志盘与数据盘**物理分离**——journaling 与 bulk 写不争盘（[ch9](../../chapter-09-disks/)）。
- 可重生的中间数据用 tmpfs——replay 缓冲、短期状态文件零盘 I/O。
- 配置/小文件读多：启动后 dentry/inode 已热，正常路径无盘 I/O——但要防内存压力把 cache 挤掉（回收见 [ch7](../../chapter-07-memory/)）。

### 衔接

- 下一节：[8.5 分析方法论](./section-8.5-分析方法论.md)（延迟分层测量）
- 关联：[ch3 OS 概念](../../chapter-03-operating-systems/)、[ch7 内存](../../chapter-07-memory/)（page cache 与回收）、[ch9 磁盘](../../chapter-09-disks/)（下层）、[06-linux-mm ch08 slab](../../../06-linux-mm/chapter-08-slab-allocator/)（dentry/inode 的物理形态）

---

### 常见陷阱

1. **write() 返回当落盘**——只写进 page cache；崩溃丢数据、fsync 才是持久化语义。
2. **free 低当内存耗尽**——page cache 是可回收内存（直到 direct reclaim 打断你，[ch7](../../chapter-07-memory/)）。
3. **nobarrier 图快**——掉电一致性换性能，生产慎用（UPS/BBU 也要想清楚）。
4. **长路径冷启动**——dentry 未热时 open 是一串元数据 I/O；高频小文件路径要短。

<details>
<summary>自测题（点击展开）</summary>

1. VFS 层对性能分析的价值？
   <details><summary>答</summary>统一观测点：不管底层什么 FS 都能在 vfs_read/write 追踪；且 syscall/VFS/FS 三层延迟对比本身就是定位法。</details>
2. write() 为什么 µs 级返回？代价是什么？
   <details><summary>答</summary>只写 page cache 标脏，writeback 异步刷——快是「记账快」；崩溃丢脏页，持久化语义在 fsync。</details>
3. ext4 三种 journal 模式的权衡？
   <details><summary>答</summary>journal（数据+元数据全日志，最慢最安全）/ordered（数据先于元数据，默认平衡）/writeback（仅元数据，最快但崩溃后可能新旧数据混合）。</details>
4. XFS 的 allocation group 为什么利于并发？
   <details><summary>答</summary>多独立分配区各有分配锁——多线程写大文件时锁不互斥（共享变分区的通用哲学）。</details>
5. tmpfs 为什么零盘 I/O？
   <details><summary>答</summary>纯内存 FS，走 VFS 接口但没有 page cache 之下的块层——读写就是内存拷贝；代价是占 RAM、重启即失。</details>

</details>


---

← [本章导读](../README.md)
