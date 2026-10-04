## 9.6 观测工具

> 章节导航：[9.5 分析方法论](./section-9.5-分析方法论.md) · 上一篇 ← · 下一篇 [9.7–9.9 可视化、实验与调优](./section-9.7-9.9-可视化实验与调优.md) · [本章导读](../README.md)

**本节讲什么**：磁盘观测的三层工具（传统统计 / BPF 逐次与直方图 / 底层硬件）、每层的关键字段与输出精读、Sloth Disk 的排查流程。

### 要点

| # | 要点 | 一句话 |
|---|------|--------|
| 1 | **统计看趋势，BPF 看分布** | iostat 的均值 vs biolatency 的直方图 |
| 2 | `biolatency -F` 分读写是**第一步** | 混着看什么也看不出 |
| 3 | `biosnoop` 抓 **outlier** | 单笔定罪 |
| 4 | `biostacks` 揪**后台发起者** | journal/kswapd/flush |
| 5 | SMART 「OK」不排除 Sloth Disk | 换盘对照是终审 |

---

### 一、传统统计层

| 工具 | 用法 | 关键字段 |
|------|------|----------|
| **`iostat -sxz 1`** | 每盘扩展统计 | `%util`、`await`、`r_await`、`w_await`、`avgqu-sz`、`r/s w/s`、`rrqm/s wrqm/s`（合并） |
| **`sar -d`** | 历史磁盘 | 事后分析、容量规划 |
| **`pidstat -d 1`** | 进程 I/O | **kB_rd/s、kB_wr/s、iodelay**（进程等 I/O 的时间） |
| **PSI** | `/proc/pressure/io` | some（有人等）/full（全员等）+ 10/60/300s 窗口 |

**iostat 输出精读**：

```
Device  r/s   w/s   rMB/s  wMB/s  rrqm/s  wrqm/s  %util  avgqu-sz  await  r_await  w_await
nvme0n1 1250  8300  4.9    32.4   0       2400    78.5   3.2        0.34   0.12     0.37
```

- `await 0.34ms`：平均每笔（读+写混算）0.34ms——**均值**，双峰会被抹平（[9.5](./section-9.5-分析方法论.md)）
- `avgqu-sz 3.2`：平均在队 3.2 笔——有排队但不算深
- `wrqm/s 2400`：写合并率 ~22%（2400/8300+2400）——合并是好事（减设备笔数）
- `w_await > r_await`：写更慢——SSD 上查 GC/预留空间，HDD 上正常
- `%util 78.5`：忙时比——NVMe 上 util 高但 await 低 = 设备并行能力强，不是饱和（**NVMe 的 util 语义弱化**：并行设备 util 高≠吞吐顶格）

**pidstat 的 iodelay** 是被低估的字段：进程因 I/O 等待的累计毫秒——「这进程本周期等了 800ms 盘」直接把 I/O 问题和应用变慢连上。

**PSI 精读**：

```
some avg10=1.23 avg60=0.45 avg300=0.12 total=8901234
full avg10=0.00 ...
```

`some > 0`：至少一个任务在等 I/O；`total` 是累计 stall µs——**比 %iowait 可靠**（不受 CPU 忙碌稀释，[9.1 陷阱](./section-9.1-9.3-核心概念与模型.md)）。

### 二、BPF / BCC 层

| 工具 | 作用 | 技巧 |
|------|------|------|
| **`biolatency`** | I/O 延迟**直方图** | **`-F`** 分 read/write/sync/flush；`-m` 毫秒桶 |
| **`biosnoop`** | 每笔 I/O 起止详情 | 抓 **outlier**、看重排序（设备完成顺序 vs 提交顺序） |
| **`biotop`** | 按进程 I/O 排行 | 谁在读盘、多大、什么类型 |
| **`biostacks`** | 块 I/O + **发起内核栈** | 揪 journal/kswapd/flush（[9.5](./section-9.5-分析方法论.md)） |

```bash
sudo biolatency-bpfcc -F -m 5      # 分类型，5ms 一轮
sudo biosnoop-bpfcc                 # 逐次：TIME(s) COMM PID DISK T SECTOR BYTES LAT(ms)
sudo biostacks-bpfcc                # 带内核栈
```

**biosnoop 输出精读**（找 outlier 的姿势）：

```
TIME(s)  COMM      PID  DISK   T  SECTOR    BYTES  LAT(ms)
1.234    strategy  4321 nvme0n1 W  83920384  4096   0.08
1.235    jbd2      305  nvme0n1 W  83924832  12288  0.11
1.987    strategy  4321 nvme0n1 W  83977216  4096   951.32   ← 单笔 951ms！
```

- 按 LAT 排序找 >100ms 的行——Sloth Disk / GC 风暴的现场证据
- `jbd2`（ext4 journal 线程）频繁出现 → journal 是 I/O 来源之一
- 提交顺序 vs 完成顺序乱序大 → 设备内部并行/重排（NVMe 正常，HDD 上的 elevator 也是）

**bpftrace 单行**（[ch15 语法](../../chapter-15-bpf/notes/section-15.2-bpftrace.md)）：

```bash
# 按进程直方图化块 I/O 大小
sudo bpftrace -e 'tracepoint:block:block_rq_issue { @bytes[comm] = hist(args->bytes); }'
# 只抓 >10ms 的慢 I/O
sudo bpftrace -e 'tracepoint:block:block_rq_complete /args->delta > 10000000/ { printf("%s %d\n", comm, args->delta/1000); }'
```

更多弹药：[附录 C](../../appendix-C-bpftrace单行命令.md)。

### 三、底层与硬件层

| 工具 | 用途 | 关键指标 |
|------|------|---------|
| **`blktrace` + `blkparse`** | 块层极细事件流（Q/G/D/C 每步时间戳） | 队列→派发→完成的逐步延迟；replay 录制源 |
| **`smartctl -a /dev/sdX`** | SMART 健康 | **重映射扇区数增长**、media wearout、NVMe 的 percentage_used |
| **`nvme smart-log`** | NVMe 专有 | busy time、unsafe shutdowns、temperature |

**blktrace 的四时间戳模型**：

```
Q（queued 入软件队列）→ G（get request 分配）→ D（dispatched 派发到驱动）→ C（completed 完成）
     └── Q→D = wait time ──┘                     └── D→C = service time ──┘
```

逐笔分解 wait/service——比 iostat 推断精确得多；代价是流量大（生产短窗口用）。

### 四、Sloth Disk 排查流程

```
症状：系统间歇性卡顿，iostat 偶发 await 飙升
  1. biolatency 长尾到 1s+？          → 确认有 outlier
  2. biosnoop：单笔 >1s 的 I/O？      → 定罪单笔（哪个扇区/多大/谁发的）
  3. smartctl -a：SMART 有错吗？      → 无错不能排除（Sloth 的定义）
  4. 同负载换盘对照（ch12 对照实验）   → 换盘后消失 = 盘的问题实锤
```

**SMART「OK」不排除 Sloth**：SMART 阈值测的是预定义故障模式（坏扇区/磨损），慢 I/O 是行为异常不是计数异常——**换盘对照是终审**（[ch16 重启实验](../../chapter-16-case-studies/)同款反证逻辑）。

### 五、工具选型速查

| 问题 | 第一工具 | 深挖 |
|------|---------|------|
| 盘忙吗？ | iostat -xz | sar -d 历史 |
| 谁在打 I/O？ | pidstat -d / biotop | biostacks（后台来源） |
| 延迟分布什么形状？ | biolatency -F | blktrace 逐步时间戳 |
| 单笔慢的证据？ | biosnoop | blktrace 该笔的 Q/D/C |
| 盘要坏吗？ | smartctl | nvme smart-log / 厂商 CLI |
| 进程被 I/O 拖累多少？ | pidstat iodelay | PSI io |

### HFT / 嵌入式关联

- **常驻三件**：`biolatency -F`（分布形态）、PSI io（stall 证据）、pidstat -d（归属）——低开销可常驻，写进巡检。
- **事件驱动三件**：尖刺触发时才跑 `biosnoop`/`biostacks`/`blktrace`（逐次级工具有观测成本，按需启用）。
- **嵌入式 eMMC**：无 SMART 标准接口——用内核 lifetime hints（`/sys/block/mmcblk*/device/life_time`）+ 写放大估算（写入量 vs 寿命消耗）替代。

### 衔接

- 上一节：[9.5 分析方法论](./section-9.5-分析方法论.md)
- 下一节：[9.7–9.9 可视化、实验与调优](./section-9.7-9.9-可视化实验与调优.md)
- 关联：[ch15 BPF](../../chapter-15-bpf/)（本层工具的原理）、[ch12 基准测试](../../chapter-12-benchmarking/)（换盘对照的实验设计）

---

### 常见陷阱

1. **拿 iostat await 当全部真相**——均值抹平双峰与长尾；分布看 biolatency，单笔看 biosnoop。
2. **NVMe 的 %util 高就当饱和**——并行设备 util 语义弱化；await/avgqu-sz 才是信号。
3. **SMART 无错就排除盘**——Sloth Disk 是行为异常，换盘对照才是终审。
4. **biosnoop 常驻生产**——逐次输出有观测成本；事件触发时短窗口使用。

<details>
<summary>自测题（点击展开）</summary>

1. iostat 的 await 和 biolatency 的区别？
   <details><summary>答</summary>await 是均值（读+写混合），双峰长尾全被抹平；biolatency 是内核聚合的对数直方图，分布形态（双峰/尾部/GC 特征）直接可见。</details>
2. NVMe 上 %util 78% 说明饱和吗？
   <details><summary>答</summary>不一定——NVMe 高度并行，util 只是「有 I/O 的时间比」；要结合 await（是否低而稳）和 avgqu-sz 判断真实压力。</details>
3. biosnoop 里 jbd2 频繁出现说明什么？
   <details><summary>答</summary>ext4 journal 线程在持续提交——journal 是 I/O 来源；结合 ch8 的 commit 间隔/大小调优，或考虑 xfs。</details>
4. blktrace 的 Q→D 和 D→C 分别量什么？
   <details><summary>答</summary>Q→D = 软件层排队等待（wait）；D→C = 派发到完成（service，含设备处理）——逐笔的精确分解。</details>
5. 怎么终审一块「卡但 SMART 正常」的盘？
   <details><summary>答</summary>同负载换盘对照实验：biosnoop 记录 outlier 模式 → 换盘重放 → 消失则原盘定罪（Sloth Disk）。</details>

</details>


---

← [本章导读](../README.md)
