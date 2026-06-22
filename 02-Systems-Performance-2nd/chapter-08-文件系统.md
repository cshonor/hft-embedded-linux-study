# Ch 8 文件系统 · File Systems

> **Systems Performance 2nd** · Brendan Gregg · **选读**

> 本章定位：**应用程序感知的往往是文件系统延迟，不是磁盘延迟** — FS 通过 page cache、预取、写回缓冲把多数逻辑 I/O 挡在内存里。Ch 7 的 page cache / file paging 在这里展开；Ch 9 磁盘是更下一层。  
> **HFT：** tick 热路径通常 **不走文件 I/O**；但 **日志、配置、历史 replay、mmap 数据文件** 仍会踩 FS — 懂「逻辑 I/O ≠ 物理 I/O」可避免误调磁盘、误杀 page cache。

---

## 大白话 · 本章就五件事

> **应用发的是逻辑 I/O，磁盘收的是物理 I/O — 中间差了一个 cache。**

**① 逻辑 I/O vs 物理 I/O — 本章第一概念。**

- App → FS = **逻辑 I/O**；FS → 磁盘 = **物理 I/O**。
- **缓存命中** → 逻辑多、物理少（通货紧缩）；**元数据更新** → 逻辑少、物理多（通货膨胀）。

**② 读靠 cache + 预取，写靠回写缓冲。**

- 未命中读进 **page cache**；顺序读触发 **read-ahead**。
- 写先进内存 **write-back**，异步刷盘 — 突发 `fsync` 仍可能卡你。

**③ 绕过 FS：O_DIRECT 与 mmap。**

- **`O_DIRECT`**：绕过 page cache，适合 DB/自管缓存。
- **mmap**：文件映射进地址空间，少 syscall；缺页仍走 FS/page cache。

**④ VFS + 三层缓存 + ext4/XFS/ZFS 选型。**

- **VFS** 统一接口 — BPF 追踪 `vfs_read` 等的好挂点。
- **Page cache / Dcache / inode cache** — `free` 里「buff/cache」大半在这里。

**⑤ 工具与调优：cachestat、ext4dist、fio、noatime。**

- BPF：**opensnoop**、**filetop**、**cachestat**、**ext4slower**。
- 基准：**fio** + 注意 WSS；调优：**noatime**、`posix_fadvise` / `madvise`。

下面按原书 8.1–8.8 展开。

---

## 8.1–8.3 核心概念与模型

### 逻辑 I/O vs 物理 I/O

```
Application                    File System                    Disk
    |  read()/write()              |                              |
    | -------- 逻辑 I/O ---------> |                              |
    |                              | ---- 物理 I/O（可能 0 次）--> |
    |                              |     （cache 命中则不发盘）      |
```

| 效应 | 含义 | 例子 |
|------|------|------|
| **I/O 通货紧缩** | 多次逻辑读 → 一次物理读 | page cache 命中、read-ahead |
| **I/O 通货膨胀** | 少量逻辑写 → 多次物理写 | 元数据 journal、小写放大 |
| **合并** | 多次逻辑写 → 一次物理写 | write-back 合并 |

**HFT 诊断：**

- 「磁盘很忙」→ 先 **`cachestat`** / **`filetop`**：是 **真刷盘** 还是 **page cache 在涨**？
- 「读配置慢一次」→ 冷 cache + read-ahead 预热；**生产应启动时预热**。

→ Ch 7 [file paging vs swap](./chapter-07-内存.md#71-72-内存核心概念)

### 缓存与缓冲

| 机制 | 行为 | 延迟 |
|------|------|------|
| **Read cache (page cache)** | 读过进内存，再读命中 | 命中 ≈ 内存速度 |
| **Write-back buffer** | 写先进 cache，标记 dirty，后台 flush | 写返回快；**掉电丢数据** 风险 |
| **Write-through** | 写同时落盘 | 慢，一致性强 |

**Gregg 观点：** FS 性能 **往往比裸盘对应用更重要** — 因为绝大多数逻辑 I/O 被 cache 吸收。

**HFT：**

- **行情 tick 路径**：不应依赖 FS read；应用内 / mmap 预热 / 共享内存。
- **异步审计日志**：write-back 友好；关键 checkpoint 才 **`fsync`** — 并预期 latency spike。

### 预取（Read-Ahead / Prefetch）

- 检测到 **顺序读** → 内核提前读后续页进 cache。
- 随机读 → 预取可能 **浪费 I/O** 且污染 cache。

```c
posix_fadvise(fd, 0, 0, POSIX_FADV_SEQUENTIAL);  // 提示顺序读
posix_fadvise(fd, off, len, POSIX_FADV_DONTNEED); // 提示可丢弃 cache
```

**HFT replay：** 顺序读历史 tick 文件时开 SEQUENTIAL；replay 完 DONTNEED 释放 cache 给 order book。

### 绕过文件系统机制

| 方式 | API / 标志 | 特点 |
|------|------------|------|
| **Direct I/O** | `O_DIRECT` | 对齐 buffer；**绕过 page cache**；自管缓存 |
| **mmap** | `mmap()` | 文件 ↔ 虚拟地址；缺页 = page fault；少 `read()` syscall |

| | 适用 | 不适用 |
|---|------|--------|
| **O_DIRECT** | DB、时序库、自研 WAL | 小随机读、依赖 kernel read-ahead |
| **mmap** | 大只读数据集、共享映射 | 需精确控制 fault 时序的热路径 |

→ [01-CSAPP Ch9 mmap](../01-CSAPP-3rd/chapter-09-虚拟内存.md) · Ch 5 [I/O 与缓冲](./chapter-05-应用程序.md#52-应用程序性能提升技术)

### 元数据（Metadata）

| 类型 | 内容 | I/O 特点 |
|------|------|----------|
| **逻辑元数据** | 权限、时间戳、目录名 | 每次 `open`/`stat`/`readdir` 可能触发 |
| **物理元数据** | inode、bitmap、journal | 小随机写，**journal 放大** |

**经典坑：** 默认 **`atime` 更新** — 每次读文件都写 inode → 无谓写 I/O。现代默认 **`relatime`**；极端可 **`noatime`** 挂载。

---

## 8.4 文件系统架构与特性

### VFS（虚拟文件系统）

```
App: read() / write() / open()
         ↓
    VFS: vfs_read() / vfs_write()  ← BPF 追踪统一点
         ↓
    ext4 / xfs / zfs / ...
         ↓
    block layer → disk
```

**性能分析：** 可在 **应用 syscall**、**VFS**、**具体 FS（ext4_xfs_*）** 各层量延迟 — 层越低越接近真实磁盘。

→ Ch 3 [VFS 概念](./chapter-03-操作系统.md)

### Linux 缓存层

| 缓存 | 存什么 | 工具 |
|------|--------|------|
| **Page Cache** | 文件 **内容**页 | `free`、`cachestat` |
| **Dentry Cache** | 路径名 → inode 查找 | `sar -v` |
| **Inode Cache** | inode 结构 / 属性 | `sar -v`、`slabtop` |

**与 Ch 7 关系：** page cache 占用 **主存** — `free` 低不代表没内存，可能是 **cache 可回收**（直到 direct reclaim）。

### 高级特性

| 特性 | 作用 | 性能 |
|------|------|------|
| **Extents** | 连续块分配，减碎片 | 顺序大文件友好 |
| **Journaling** | 崩溃一致性 | 元数据 journal 增写放大 |
| **COW** | 快照、克隆 | btrfs/ZFS；写路径可能变复杂 |

### 常见文件系统（Linux）

| FS | 特点 | HFT 场景 |
|----|------|----------|
| **ext4** | 默认、成熟 | 系统盘、日志盘 |
| **XFS** | 大文件、并行分配组、延迟分配 | 大容量日志 / 数据归档 |
| **ZFS** | ARC 缓存、存储池、recordsize | 非 tick 路径；调 recordsize 匹配 I/O |
| **btrfs** | COW、快照 | 备份、开发环境 |

**HFT 实践：** 系统盘 ext4/xfs + **`noatime`**；NVMe 日志盘与数据盘 **分离**，避免 journaling 与 bulk 写争抢。

---

## 8.5 分析方法论

### 延迟分析（Latency Analysis）

**测量层次：**

```
① 应用事务计时（端到端）
② syscall 层（strace / BPF 测 read/write 耗时）
③ VFS 层（vfs_read/write 追踪）
④ FS 层（ext4_file_read_iter / xfs_* 延迟直方图）
```

**事务成本（Transaction Cost）：**

```
事务成本 = 事务总时间中阻塞在 FS I/O 上的比例
```

若 < 1% — FS 不是瓶颈；若 > 10% — 查 cache 命中、慢 fsync、元数据风暴。

→ Ch 2 [延迟分解](./chapter-02-方法论.md#27-延迟分析与分解)

### 工作负载特征

| 维度 | 问什么 | 工具 |
|------|--------|------|
| **IOPS** | 每秒多少次 I/O | iostat、`filetop` |
| **吞吐量** | MB/s | `filetop`、sar |
| **I/O 大小** | 4K vs 1M | `biosnoop`、fio |
| **读/写比** | 读多还是写多 | sar、BPF |
| **随机/顺序** | 预取是否有效 | fio `--rw=randread` vs `read` |

### 微基准测试注意：WSS

| 测试集大小 | 实际测到的是 |
|------------|--------------|
| **WSS << RAM** | **page cache 性能** — 极快，误导 |
| **WSS >> RAM** | 磁盘 + FS 真实路径 |
| **O_DIRECT** | 绕过 cache，测磁盘/FS 直连 |

```bash
# 清空 page cache（仅测试环境！生产禁止）
echo 3 | sudo tee /proc/sys/vm/drop_caches
```

→ [Ch 12 基准测试](./chapter-12-基准测试.md)

---

## 8.6 观测工具

### 传统统计工具

| 工具 | 用途 |
|------|------|
| **`mount`** | 挂载选项：`noatime`、`data=writeback` 等 |
| **`free` / `top` / `vmstat`** | cache 占用；与 Ch 7 联动 |
| **`sar -v`** | dentry/inode cache 统计 |
| **`slabtop`** | dentry/inode 等 slab 占用 |

### BPF / BCC 工具集

| 工具 | 作用 | HFT 场景 |
|------|------|----------|
| **`opensnoop`** | 谁 open 了什么文件 | 找意外读配置、权限问题 |
| **`filetop`** | 按文件 I/O 吞吐排序 | 哪份日志/数据文件在狂读写的 |
| **`cachestat`** | page cache **命中率** | 区分 cache 命中 vs 真读盘 |
| **`ext4dist` / `xfsdist`** | FS 操作延迟直方图 | 看双峰（快 cache / 慢 disk） |
| **`ext4slower` / `xfsslower`** | 超过阈值（如 10ms）的慢操作 | 抓 fsync、journal 尖刺 |
| **bpftrace VFS 单行** | 追踪 `vfs_read` 等 | 附录 C 扩展 |

```bash
# 页缓存命中情况（需 BCC）
sudo cachestat-bpfcc 5

# 慢 ext4 操作 > 10ms
sudo ext4slower-bpfcc 10

# 谁在 open 文件
sudo opensnoop-bpfcc
```

→ [Ch 15 BPF](./chapter-15-BPF技术.md) · [附录 C](./appendix-C-bpftrace单行命令.md) · [03-BPF](../03-BPF-Performance-Tools/)

---

## 8.7–8.8 实验与调优

### fio 基准测试

**fio** = 灵活 I/O 测试器 — 支持随机分布、延迟分位（P99、P99.99）、多线程。

```bash
fio --name=seqread --filename=/data/testfile --size=32G \
    --rw=read --bs=1M --direct=1 --ioengine=libaio \
    --runtime=60 --time_based --group_reporting
```

| 参数 | 含义 |
|------|------|
| **`--direct=1`** | O_DIRECT，测磁盘非 cache |
| **`--size`** | 必须 **> RAM** 才测真磁盘（否则测 cache） |
| **`--bs`** | 块大小 — 对齐应用真实 I/O |
| **`--rw`** | read/write/randread/randwrite |

**HFT：** 上线前对 **日志盘** 单独 fio — 确认与 NVMe 数据面 **不共享瓶颈**。

### 应用层调优

| API | 作用 |
|-----|------|
| **`posix_fadvise()`** | 顺序/随机、willneed、dontneed |
| **`madvise()`** | mmap 区域：SEQUENTIAL、RANDOM、DONTNEED |
| **`sync_file_range()`** | 范围刷盘（细粒度控制） |

**原则：** 给内核 **正确 hint** 比盲目增大 buffer 更有效。

### 挂载与 FS 参数

| 选项 / 参数 | 效果 |
|-------------|------|
| **`noatime` / `relatime`** | 减少读触发的元数据写 |
| **`barrier` / `nobarrier`** | 一致性 vs 性能 — **生产慎用 nobarrier** |
| **ext4 `data=ordered`** | 默认平衡 |
| **XFS `allocsize`** | 预分配减少碎片 |
| **ZFS `recordsize`** | 匹配应用 I/O 大小（如 128K） |

**HFT 日志盘示例（思路，按合规调整）：**

```
UUID=... /var/log/hft  xfs  noatime,nodiratime,logbufs=8  0 2
```

### USE 方法（File System 视角）

| 字母 | 问什么 |
|------|--------|
| **U** | cache 利用、FS 层 CPU |
| **S** | 慢 I/O 队列、应用阻塞在 read/write/fsync |
| **E** | I/O error、只读 remount |

---

## 本章 Checklist

- [ ] 能解释 **逻辑 I/O vs 物理 I/O**、通货紧缩/通货膨胀
- [ ] 知道 **page cache / write-back** 与 Ch 7 `free` 的关系
- [ ] 会用 **`cachestat`** 或等价手段看 cache 命中率
- [ ] 跑过 **fio** 且测试集 **大于 RAM** 或 **`direct=1`**
- [ ] 检查挂载选项：**atime**、日志盘是否与数据面分离
- [ ] 热路径确认：**无 sync 写、无冷读大文件**

---

## HFT 精读捷径（Ch 8 在路线中的位置）

```
Ch 7  内存 — page cache 占 RAM
Ch 8  文件系统（本章：逻辑/物理 I/O、VFS、cache、BPF 工具）
  → Ch 9  磁盘（cache 未命中之后）
  → Ch 5  mmap / O_DIRECT 应用用法
  → Ch 12 fio 方法论
```

**HFT 读法：**

| 场景 | 建议 |
|------|------|
| **tick / 发单热路径** | ⚪ 不应依赖 FS — 确认无意外 `open`/`write` |
| **日志、配置、replay** | 🟡 精读 8.1–8.3 + 8.6 `filetop`/`ext4slower` |
| **mmap 历史数据** | 🟡 8.3 + Ch 7 缺页 + `madvise` |

**本章最小行动集（非热路径机器）：**

1. **`mount | grep`** — 记录 `noatime`/FS 类型。
2. **`sudo cachestat-bpfcc 5`** — 看 page cache 命中比例。
3. **`sudo opensnoop-bpfcc`** 启动策略 30 秒 — 有无意外文件访问。
4. 日志盘 **`fio --direct=1`** 一次，留 baseline。

**Gregg 本章金句（HFT 版）：**

> 应用感受到的是 **文件系统延迟**，不是磁盘延迟 — 先查 **cache 命中**，再查磁盘。  
> **逻辑 I/O 多 ≠ 磁盘忙**；元数据和 atime 可以让磁盘 **比你想的更忙**。

---

## 相关章节

- 上一章：[chapter-07-内存.md](./chapter-07-内存.md)
- 下一章：[chapter-09-磁盘.md](./chapter-09-磁盘.md)
- OS / VFS：[chapter-03-操作系统.md](./chapter-03-操作系统.md)
- 应用 I/O：[chapter-05-应用程序.md](./chapter-05-应用程序.md)
- 基准测试：[chapter-12-基准测试.md](./chapter-12-基准测试.md)
- BPF：[chapter-15-BPF技术.md](./chapter-15-BPF技术.md)
- LKD 页缓存：[05-LKD ch16](../05-Linux-Kernel-Development/00_Book_3rd_Notes/chapter-16-页高速缓存和页回写.md)
- 全书目录：[OUTLINE.md](./OUTLINE.md)
