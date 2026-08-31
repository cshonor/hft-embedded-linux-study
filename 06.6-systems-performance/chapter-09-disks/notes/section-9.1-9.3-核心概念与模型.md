## 9.1–9.3 核心概念与模型

> 章节导航：[本章导读](../README.md) · 下一篇 [9.4 硬件与软件架构](./section-9.4-硬件与软件架构.md)

**本节讲什么**：磁盘 I/O 的时间模型（request = wait + service）、各介质的延迟量级表、I/O 特征四维度、以及两大指标陷阱（虚拟磁盘 util、%iowait 稀释）的机制解释。

### 要点

| # | 要点 | 一句话 |
|---|------|--------|
| 1 | I/O 时间 = **队列等待 + 设备处理** | 两部分要分开归因 |
| 2 | 固件内部还有排队 | OS 测到的 service time ≠ 物理时间 |
| 3 | **IOPS 不带四要素 = 废话** | 随机/顺序、读写、块大小、队列深度 |
| 4 | 虚拟盘 util 会**撒谎** | 阵列 100% util ≠ 全部成员满 |
| 5 | %iowait 会被 **CPU 忙碌稀释** | PSI + await 更可靠 |

---

### 一、I/O 时间模型

```
I/O Request Time（端到端）
    = Wait Time（队列等待）
    + Service / Response Time（设备处理，含盘内队列）
```

| 术语 | 含义 | 谁量 |
|------|------|------|
| **Request time** | 发 I/O → 完成 | 应用（syscall 返回）/ 块层 |
| **Wait time** | 在 OS（blk-mq）或 HBA 队列中等待 | iostat `await` 中由 `avgqu-sz` 推断 |
| **Service / Response time** | 设备侧耗时 | `r_await`/`w_await`；BPF 直方图精确分解 |

**注意**：磁盘固件内部还有排队（NVMe 每盘 64K+ 队列深度）——OS 测到的 service time **不是**纯机械/闪存物理时间，统称 **disk response time / latency**。要拆开真物理时间得用设备侧工具（smartctl 的 latency log、厂商 CLI）。

**归因意义**：await 高时先分清是 wait 高（系统排队 → 降负载/调队列/查邻居）还是 service 高（设备慢 → 查盘健康/降并发）——两种结论的动作完全不同。

### 二、时间尺度（量级感）

| 场景 | 典型延迟 | 相对 CPU 周期 |
|------|----------|--------------|
| NVMe 读（无排队） | **~20–100 µs** | 数万周期 |
| SATA SSD 读 | 100–200 µs | |
| HDD 顺序读 | ~1 ms | |
| HDD 随机读 | **~8 ms+**（寻道 4ms + 转速 4ms@7200rpm） | |
| 队列饱和 + 控制器 | **100 ms – 1 s+** | |
| **Sloth Disk**（故障盘） | 个别 I/O **> 1 s**，无明确 SMART 错 | |

两个推论：
1. **介质跨度 5 个数量级**——选错介质比任何调优都贵；HFT 热路径机器**不应有常态 ms 级块 I/O**。
2. **排队把延迟放大 1000×**——深队列追求吞吐时，单笔延迟从 µs 级被推到百 ms 级（这就是 [排队论](../../chapter-02-methodologies/) 的 M/M/1 尾部）。

**HFT**：P99 tick 尖刺若与 block I/O 时间对齐——先 `biosnoop` 找 outlier I/O（谁发的、多大、多慢），再 `smartctl` 查盘。

### 三、I/O 特征四维度

| 维度 | 影响 |
|------|------|
| **随机 vs 顺序** | HDD 随机极慢（寻道）；SSD 仍受 FTL 映射/GC 影响（顺序写对 GC 友好） |
| **读 vs 写** | SSD 写常更慢（erase-write + WA）；sync/flush 最慢（强制下刷） |
| **I/O 大小** | 4K 随机 vs 1M 顺序——**IOPS 与 MB/s 不可互换**（4K@100k IOPS = 400MB/s） |
| **队列深度** | 深度↑ 吞吐↑ 但**延迟↑**——低延迟系统**控队列深度**（iodepth=1 甚至更激进的同步直写） |

**IOPS Are Not Equal（Gregg）**——"5000 IOPS" 必须附带：

```
  - 随机 or 顺序？
  - 读 or 写？
  - 块大小？
  - 队列深度？
  - 是否 O_DIRECT / 是否绕过 cache？
```

同一个盘：4K 随机读 QD1 ~20k IOPS；1M 顺序读 QD32 换算成 4K 等效可能上百万——**不带条件的 IOPS 是营销话术**（与 [ch12 拷问](../../chapter-12-benchmarking/notes/section-12.4-基准测试拷问Benchmark-Questions.md) 直接呼应）。

### 四、指标陷阱

**陷阱 1：虚拟磁盘使用率**

RAID/SAN 呈现**单块 `sdX`**——`iostat` 100% util 可能只是**部分成员盘满**，其他盘空闲。拆穿方法：`avgqu-sz` 与 await 的组合（真满的阵列 await 也会涨）、以及阵列管理工具（`MegaCli`、厂商 CLI）看物理盘各自负载。

**陷阱 2：%iowait 的稀释效应**

```
%iowait = CPU 时间中「空闲 且 至少有一个 I/O 未完成」的比例
```

| 误解 | 真相 |
|------|------|
| iowait 低 = 磁盘快 | CPU 上若有**其他计算任务**，空闲窗口消失——iowait **被稀释**（极端：CPU 满载时 iowait=0 而盘已跪） |
| iowait 高 = 磁盘慢 | 可能——但要结合 await、PSI、biolatency |

%iowait 的分母是 CPU 空闲时间——它测的是「CPU 闲着等 I/O 的机会成本」，不是磁盘健康度。**多核机器上一核忙，全机 iowait 被稀释**。

**更可靠的替代**：`/proc/pressure/io`（PSI，直接量 I/O stall 的时间比例与最坏窗口）、`iostat await`、BPF 延迟直方图（[9.6](./section-9.6-观测工具.md)）。

### 五、一次 read() 的延迟分解（全栈视角）

应用感知的 100µs 里，各层各占多少——这是后面 9.5 延迟分析的总纲：

```
read() 返回耗时 100µs（例）
 ├─ syscall 进出 + VFS 查找          ~1-2µs
 ├─ page cache 命中？                → 直接返回（快路径到此为止）
 ├─ cache miss → bio 构造            ~1µs
 ├─ blk-mq 入队 + 调度（wait time）  0 ~ ∞（负载相关！）
 ├─ 驱动提交 + PCIe 传输             ~2-5µs
 ├─ 设备处理（service time）         20-80µs（NVMe）
 └─ 中断/轮询 + 唤醒 + copy 到用户    ~5-20µs（含调度器延迟）
```

**关键洞察**：除设备处理外，每一层都受**系统当时状态**影响——调度延迟、中断合并、CPU 迁移。盘没变，I/O 延迟也能差 10×——这就是为什么延迟归因必须分层（[9.5 方法论](./section-9.5-分析方法论.md)、[ch16 直方图判读](../../chapter-16-case-studies/)）。

### HFT / 嵌入式关联

- **热路径零同步盘 I/O**：策略线程的任何 `fsync`/同步读都引入 ms 级不可控尾——日志异步化 + 预读 mmap 是标配。
- **日志盘独立**：日志突发写不与 replay 读争队列（同一 NVMe 的 GC 会被写流量触发，读延迟连带变差）。
- **PCIe 带宽规划**：数据面网卡与日志盘争 PCIe lane——高 pps 时网卡 DMA 会挤压盘的提交路径（[9.4](./section-9.4-硬件与软件架构.md)）。
- **嵌入式**：eMMC/UFS 的 FTL GC 停顿是车载/工控卡顿的经典来源——写入模式（顺序化、预留 OP 空间）比换件更有效。

### 衔接

- 下一节：[9.4 硬件与软件架构](./section-9.4-硬件与软件架构.md)（HDD/SSD 机制 + blk-mq）
- 关联：[ch8 文件系统](../../chapter-08-file-systems/)（page cache 层）、[ch12 基准测试](../../chapter-12-benchmarking/)（fio 口径）、[ch2 排队论](../../chapter-02-methodologies/)（队列深度与延迟的数学关系）

---

### 常见陷阱

1. **比较不带条件的 IOPS**——4K 随机 QD1 与 1M 顺序 QD32 的「IOPS」差两个数量级，不可比。
2. **util=100% 就断言盘满**——虚拟盘 util 会撒谎；结合 await/avgqu-sz/物理盘工具。
3. **iowait=0 就排除盘问题**——CPU 忙碌稀释效应；看 PSI io 与 await。
4. **深队列压测的延迟当生产延迟**——QD32 的 P99 必然烂；低延迟口径是 QD1。

<details>
<summary>自测题（点击展开）</summary>

1. await 高，怎么区分是排队还是盘慢？
   <details><summary>答</summary>看 avgqu-sz（队列长度）与 await 的关系：avgqu-sz 高 → wait 为主（系统问题）；avgqu-sz 低而 await 高 → service 为主（设备问题，查 smartctl/换盘）。</details>
2. 为什么 OS 测到的 service time 不是设备物理时间？
   <details><summary>答</summary>固件内部还有排队（NVMe 队列深 64K+）——OS 看到的是提交到完成，中间含盘内调度；真物理时间要设备侧工具。</details>
3. %iowait 的定义和稀释机制？
   <details><summary>答</summary>「CPU 空闲且有未完成 I/O」的时间比例——分母是空闲时间，任何 CPU 计算负载都会压缩空闲、稀释 iowait；CPU 满载时盘跪了 iowait 也是 0。</details>
4. 低延迟系统为什么控制队列深度？
   <details><summary>答</summary>队列换吞吐：深度↑ 吞吐↑ 但每笔等待↑（排队论尾部）；延迟敏感用 QD1 同步路径，吞吐敏感用深队列——两者不可兼得。</details>
5. HFT 热路径机器出现常态 ms 级块 I/O 说明什么？
   <details><summary>答</summary>架构问题：要么同步 I/O 泄漏进热路径（fsync/同步读），要么日志与热路径争盘——先 biosnoop 定位来源。</details>

</details>


---

← [本章导读](../README.md)
