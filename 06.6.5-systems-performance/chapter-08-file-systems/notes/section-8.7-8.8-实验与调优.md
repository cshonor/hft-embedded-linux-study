## 8.7–8.8 实验与调优

> 章节导航：[8.6 观测工具](./section-8.6-观测工具.md) · 上一篇 ← · [本章导读](../README.md)

**本节讲什么**：fio 做 FS 层基准的口径、应用层 I/O advise API（fadvise/madvise/sync_file_range）、挂载与 FS 参数的语义、FS 视角的 USE 检查。

### 要点

| # | 要点 | 一句话 |
|---|------|--------|
| 1 | fio 的 `--size` 必须 **> RAM** | 否则测 cache（[ch12](../../chapter-12-benchmarking/) 同坑） |
| 2 | **正确 hint 比大 buffer 有效** | fadvise/madvise 让内核按你的访问模式优化 |
| 3 | `noatime` 是免费午餐 | 每次读省一次元数据写 |
| 4 | nobarrier 是**一致性换性能** | 生产慎用 |
| 5 | ZFS 的 `recordsize` 要**匹配 I/O 大小** | 错配 = 读放大或写放大 |

---

### 一、fio 基准测试（FS 层口径）

```bash
fio --name=seqread --filename=/data/testfile --size=32G \
    --rw=read --bs=1M --direct=1 --ioengine=libaio \
    --runtime=60 --time_based --group_reporting \
    --percentile_list=50:99:99.9
```

| 参数 | 含义 | 口径陷阱 |
|------|------|---------|
| **`--direct=1`** | O_DIRECT | 不加 = 测 page cache（除非 size >> RAM） |
| **`--size`** | 测试集大小 | **必须 > RAM** 才测真磁盘 |
| **`--bs`** | 块大小 | **对齐应用真实 I/O**（日志 4K？归档 1M？） |
| `--rw` | read/write/randread/randwrite | 随机/顺序测的是不同东西（[ch9](../../chapter-09-disks/notes/section-9.7-9.9-可视化实验与调优.md)） |
| `--ioengine` | libaio/io_uring | 引擎不同 = 提交路径不同 |

**FS 层 vs 裸盘层**：fio 挂**文件**（`/data/testfile`）测的是 FS+盘整条路径；挂**块设备**（`/dev/nvme1n1`，direct）绕过 FS——两次差值就是 FS 开销（journal/分配/元数据）。HFT 日志盘验收建议两个都跑。

**HFT**：上线前对**日志盘**单独 fio——确认与 NVMe 数据面不共享瓶颈（PCIe/GC，[ch9.4](../../chapter-09-disks/notes/section-9.4-硬件与软件架构.md)）。

### 二、应用层调优：advise API

| API | 作用 | 典型用法 |
|-----|------|---------|
| **`posix_fadvise()`** | 文件访问模式提示 | `POSIX_FADV_SEQUENTIAL`（开预读）/`FADV_RANDOM`（关预读）/`FADV_DONTNEED`（读后弃缓存） |
| **`madvise()`** | mmap 区域提示 | `MADV_SEQUENTIAL`/`MADV_RANDOM`/`MADV_DONTNEED`（释放）/`MADV_HUGEPAGE`（THP，见 [06-linux-mm](../../../06-linux-mm/chapter-03-page-table-management/notes/note-透明大页THP.md)） |
| **`sync_file_range()`** | 范围刷盘 | 细粒度控制——刷 10MB 文件中间那 1MB，不用全文件 fsync |
| `fallocate()` | 预分配 | 消除写时分配延迟与碎片 |

**原则：给内核正确 hint 比盲目增大 buffer 更有效**。两个高价值场景：

1. **随机读 mmap**：默认预读会把无用邻居页灌进 cache——`madvise(MADV_RANDOM)` 关掉，cache 利用率立刻改善（cachestat 验证）。
2. **一次性大文件处理**：`fadvise(DONTNEED)` 处理完就弃——防止把热数据挤出 page cache（对同机其他进程是公益）。

`sync_file_range` 的低延迟用法：日志线程攒批后**分区段刷**（每段独立 sync_file_range）——单段失败不影响全局，且把一次大 fsync 拆成多个小窗口。

### 三、挂载与 FS 参数

| 选项/参数 | 效果 | 注意 |
|-----------|------|------|
| **`noatime` / `nodiratime`** | 读文件不再更新 atime——**每次读省一次元数据写** | 免费午餐；relatime 是折中默认 |
| `barrier` / `nobarrier` | 写屏障——掉电时 journal 顺序保证 | **nobarrier = 一致性换性能**，生产慎用（UPS/BBU 下再考虑） |
| ext4 `data=ordered/writeback/journal` | journal 模式 | [8.4](./section-8.4-文件系统架构与特性.md) |
| ext4 `commit=` | journal commit 间隔（默认 5s） | 加大 = 攒批更久，崩溃窗口更大 |
| ext4 `journal_dev=` | journal 放独立设备 | 日志/数据分盘的经典实现 |
| XFS `allocsize` / `swalloc` | 预分配 | 减碎片 |
| ZFS `recordsize` | 记录块大小（默认 128K） | **匹配应用 I/O**：DB 8K/16K；日志 128K+ |
| `noexec/nosuid/nodev` | 安全 | 不影响性能但 runbook 该有 |

**ZFS recordsize 错配的代价**：应用每次读 8K 而 recordsize=128K——**读放大 16×**（读一整个 record）；应用每次写 8K——**写放大**（COW 整个 128K record 重写）。调对 recordsize 是 ZFS 性能的第一杠杆。

**HFT 日志盘示例**：

```
UUID=... /var/log/hft  xfs  noatime,nodiratime,logbufs=8  0 2
```

### 四、USE 方法（File System 视角）

| 字母 | 问什么 | 工具 |
|------|--------|------|
| **U** | cache 利用、FS 层 CPU | `cachestat`、ext4dist |
| **S** | 慢 I/O 队列、应用阻塞在 read/write/fsync | `ext4slower`、PSI io、`pidstat -d` 的 iodelay |
| **E** | I/O error、只读 remount（journal 损伤后） | `dmesg`、`df`（只读挂载检查） |

### 五、调优优先级

| 优先级 | 手段 | 成本 |
|--------|------|------|
| 1 | 挂载选项：`noatime` | 零 |
| 2 | 应用 hint：fadvise/madvise/攒批 | 代码小改 |
| 3 | journal 策略：commit 间隔/分盘 | 配置 |
| 4 | recordsize/allocsize 匹配 | 重建 FS |
| 5 | 换 FS / 换盘 | 预算 |

### HFT / 嵌入式关联

- **日志写路径的最终形态**：定长记录 + 环形单文件（元数据零开销）+ 攒批 + sync_file_range 分段刷 + noatime 挂载——这条链上每一环都有本节的依据。
- **回放读路径**：mmap + MADV_SEQUENTIAL + 预读（或一次性整文件读进 tmpfs）——cache 命中恒 100%（cachestat 验证）。
- **合规 vs 性能**：fsync 频率是合规决策（[8.5](./section-8.5-分析方法论.md)）——runbook 里写明「允许丢失的窗口」并让 journal commit 间隔与之对齐。
- **嵌入式 flash**：atime 更新在 eMMC 上是**写寿命税**——noatime 从优化变成必选。

### 衔接

- 上一节：[8.6 观测工具](./section-8.6-观测工具.md)
- 关联：[ch9 fio 口径](../../chapter-09-disks/notes/section-9.7-9.9-可视化实验与调优.md)、[ch12 基准拷问](../../chapter-12-benchmarking/notes/section-12.4-基准测试拷问Benchmark-Questions.md)、[06-linux-mm THP](../../../06-linux-mm/chapter-03-page-table-management/notes/note-透明大页THP.md)、[14-HFT 工程实践](../../../14-hft-engineering/)
- 下一章：[ch9 磁盘](../../chapter-09-disks/)

---

### 常见陷阱

1. **fio size 小于 RAM**——测的是 cache 带宽，报告极快（[ch12 失败模式](../../chapter-12-benchmarking/)）。
2. **随机读不 madvise**——默认预读污染 cache，命中率虚低。
3. **nobarrier 上生产**——掉电后 journal 可能失效、FS 损伤；性能收益远小于风险。
4. **ZFS 默认 recordsize 直接用**——8K 访问模式配 128K record = 16× 读放大。
5. **忘了 noatime**——每次读文件一次元数据写，eMMC 上直接烧寿命。

<details>
<summary>自测题（点击展开）</summary>

1. fio 挂文件和挂块设备的差值是什么？
   <details><summary>答</summary>FS 开销——journal、块分配、元数据维护；两轮对比即可量化 FS 层成本。</details>
2. madvise(MADV_RANDOM) 什么时候用？
   <details><summary>答</summary>mmap 随机访问时关掉默认预读——防止无用邻居页灌满 cache（改善命中率与回收压力）。</details>
3. sync_file_range 相比 fsync 的优势？
   <details><summary>答</summary>范围粒度——只刷指定区间，把一次大 fsync 拆成多个小窗口，且不强制元数据 journal 语义（按需组合）。</details>
4. ZFS recordsize 错配的两个代价？
   <details><summary>答</summary>读放大（小读拉整 record）+ 写放大（COW 整 record 重写）——都必须匹配应用 I/O 大小。</details>
5. 为什么嵌入式 eMMC 上 noatime 是必选？
   <details><summary>答</summary>atime 更新是每次读都触发的元数据写——消耗 flash 写寿命（有擦写上限），不只是性能问题。</details>

</details>


---

← [本章导读](../README.md)
