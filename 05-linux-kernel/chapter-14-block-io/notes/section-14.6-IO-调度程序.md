## ⑥ I/O 调度程序 · I/O Schedulers

**磁盘寻道**（机械盘移磁头）极慢 — 若 **FIFO 直发** → 性能极差。

**I/O 调度器（电梯调度器）** 对队列中请求：

| 手段 | 目的 |
|------|------|
| **合并（merging）** | 相邻扇区多请求 **合成一个** |
| **排序（sorting）** | 按 **物理扇区顺序** 排列 — **减寻道** |

#### 书中主要调度器

| 调度器 | 要点 |
|--------|------|
| **Linus 电梯** | 2.4 默认 · 合并+排序 · **年龄阈值** 防饥饿 — **效率不高** |
| **Deadline** | 除位置队列外，**读 FIFO（默认 500ms）**、**写 FIFO（默认 5s）** — 超时 **优先服务** → 防 **写饿死读** · 牺牲部分吞吐换 **低读延迟** |
| **Anticipatory（预测）** | 在 Deadline 上加 **启发式**：读完 **等 ~6ms** 猜下一块相邻读 — 预测成功则 **少两次寻道** |
| **CFQ（完全公平排队）** | **每进程一队列** · **轮转** 服务 — 书中称 **默认** · 桌面/多媒体友好 |
| **Noop** | **几乎不排序** · 只做 **简单合并** — 给 **SSD/闪存**（无机械寻道） |

```
HDD 时代思维：
  随机到达的 read/write 请求
        ▼
  电梯：合并相邻 + 按柱面排序
        ▼
  磁头少走冤枉路 → 吞吐↑
```

| 设备 | 倾向 |
|------|------|
| **机械 HDD** | deadline / CFQ（历史） |
| **SSD / NVMe** | **noop / none** — 无寻道，排序反而添延迟 |

### 版本断崖：书里讲的五个调度器，v6.6 里一个都不在

| LKD 讲的 | v6.6 现状 | 说明 |
|---|---|---|
| **Linus 电梯**（2.4 默认） | 早已删除 | 只有考古价值 |
| **Deadline** | → **mq-deadline** | 单队列版随 blk-mq 落地被删，只剩 blk-mq 版本。**超时语义原样保留** |
| **Anticipatory（预测）** | **v2.6.33 删除** | 先被 CFQ 吸收，后随 CFQ 一起退役 |
| **CFQ**（书中称默认） | **v5.0 随单队列路径删除** | 精神续作是 **BFQ** |
| **Noop** | → **none** | 连名字都改了 |

```bash
cat /sys/block/nvme0n1/queue/scheduler
# [none] mq-deadline kyber bfq
```

**但核心思想没变**。mq-deadline 的参数与 LKD 描写完全一致（mq-deadline.c:30-38）：

```c
static const int read_expire  = HZ / 2;  /* 读超时 500ms */
static const int write_expire = 5 * HZ;  /* 写超时 5s（软限制！） */
static const int writes_starved = 2;     /* 读最多饿写 2 次 */
static const int fifo_batch = 16;        /* 16 个连续请求当一批处理 */
```

→ **算法思路照搬，只是换到了 blk-mq 框架上。**

### 四个现代调度器对比

| 调度器 | **设计目标** | 机制 | 适用 |
|---|---|---|---|
| **none** | 零开销 | 不排序，只在 plug 里做简单合并 | **多队列设备（NVMe）默认** |
| **mq-deadline** | 防饿死 + 读优先 | 位置排序 + 读/写 FIFO 超时 | 单队列设备默认；HDD；读延迟敏感 |
| **kyber** | **控制尾延迟** | 按请求类型分 domain，**自适应队列深度** | 快速设备上主动压制 p99 |
| **bfq** | 公平 + 交互响应 | 按进程分配**扇区预算**（非时间片） | 桌面/交互/多租户 |

### kyber：四个调度器里唯一以「延迟」为直接目标（HFT 重点）

源码头部注释（kyber-iosched.c:3）：

```c
/*
 * The Kyber I/O scheduler. Controls latency by throttling queue depths using
 * scalable techniques.
 */
```

**它通过限制队列深度来控制延迟** —— 其他三个的目标分别是零开销、防饿死、公平，只有 kyber 直接瞄准延迟。

四个调度域（kyber-iosched.c:29），每域有独立的**深度上限**与**延迟目标**：

```c
enum { KYBER_READ, KYBER_WRITE, KYBER_DISCARD, KYBER_OTHER, KYBER_NUM_DOMAINS };

static const unsigned int kyber_depth[] = {
	[KYBER_READ]    = 256,
	[KYBER_WRITE]   = 128,
	[KYBER_DISCARD] = 64,
	[KYBER_OTHER]   = 16,
};

static const u64 kyber_latency_targets[] = {
	[KYBER_READ]    = 2ULL  * NSEC_PER_MSEC,   /*  2 ms */
	[KYBER_WRITE]   = 10ULL * NSEC_PER_MSEC,   /* 10 ms */
	[KYBER_DISCARD] = 5ULL  * NSEC_PER_SEC,    /*  5 s  */
};
```

**自适应机制**：kyber 用直方图统计各域的完成延迟，超标就**收缩**该域队列深度（限流），好转后再逐步放开。源码注释点出了它敢设上限的底气：

```
 * Even for fast devices with lots of tags like NVMe, you can saturate the
 * device with only a fraction of the maximum possible queue depth.
```

**这条对 HFT 极关键**：NVMe 队列开到 1024 深时，设备内部排队会让 p99 暴涨。kyber 主动把深度压到「刚好能饱和设备」的水平，用少量吞吐换回尾延迟。

防同步饿死：`KYBER_ASYNC_PERCENT = 75` —— 保留 25% 请求额度给同步操作，防止异步请求洪泛把同步请求饿死。

### BFQ：从「时间域」换到「服务量域」

源码注释（bfq-iosched.c:24）：

```
 * BFQ is a proportional-share storage-I/O scheduling algorithm based
 * on the slice-by-slice service scheme of CFQ. But BFQ assigns
 * budgets, measured in number of sectors, to processes instead of
 * time slices. ... This change from the time to the service domain enables BFQ
 * to distribute the device throughput among processes as desired,
 * without any distortion due to throughput fluctuations, or to device
 * internal queueing.
```

CFQ 给**时间片**，但设备吞吐会波动（GC、磨损均衡、内部队列），同样的时间片服务量天差地别 → 公平性失真。BFQ 给**扇区预算**，服务量恒定，公平性不受设备内部行为干扰。默认最大预算 `bfq_default_max_budget = 16 * 1024` 扇区。

### 为什么 SSD/NVMe 不需要排序（修正一个常见说法）

常见说法是「NVMe 多队列硬件完全绕过 IO 调度器」。**准确说法**：绕过是**默认 none** 的结果，不是硬件强制的 —— 你仍然可以挂 mq-deadline / kyber，只要你想。

| 排序的收益来源 | HDD | SSD / NVMe |
|---|---|---|
| 磁头寻道 | 毫秒级，排序收益巨大 | 不存在 |
| 旋转延迟 | 存在 | 不存在 |
| **排序的代价** | 相对收益可忽略 | **CPU 开销 + 排队延迟，纯亏** |

另外注意：**none 不等于零调度**。plug 攒批时的简单合并仍在（见 14.5），只是不做跨请求的位置排序。

### 选型决策

| 场景 | 选 | 理由 |
|---|---|---|
| HFT 落盘（尾延迟优先） | **none** 或 **kyber** | none 零开销；kyber 主动压制 p99 |
| HFT + 混合读写（读延迟敏感） | **mq-deadline** | 读 500ms 超时，防写饿死读 |
| 机械 HDD / 单队列设备 | mq-deadline | 这是它的默认场景 |
| 多租户 / cgroup 配额 | bfq | 按权重分配吞吐 |
| 桌面交互 | bfq | 交互响应好 |

**HFT / 排障：**

| 工具 | 看什么 |
|------|--------|
| **`iostat -x`** | `await`、利用率、队列长度 |
| **`biolatency`（BCC）** | 块 I/O 延迟分布 · read/write/sync 分开 |
| **`biosnoop`（BCC）** | 单笔 IO 的延迟与偏移，定位具体慢请求 |
| **`/sys/block/*/queue/scheduler`** | 确认当前挂的是哪个 |
| **`blktrace` + `blkparse`** | 拆解块层各阶段耗时（Q→G→I→D→C） |

→ [06.6 SysPerf Ch9 §9.4](../../../06.6-systems-performance/chapter-09-disks/notes/section-9.4-硬件与软件架构.md) · [Ch15 bpf biolatency](../../../06.6-systems-performance/chapter-15-bpf/)



<details>
<summary>自测题（点击展开）</summary>

**Q1.** Deadline、CFQ、BFQ、none 调度器分别适合什么场景？

<details><summary>答案</summary>

Deadline：保证请求不饿死（每个请求有超时），适合数据库/服务器。CFQ（Completely Fair Queueing）：按进程公平分配 IO 带宽，适合桌面。BFQ：CFQ 改进版，更适合交互/低延迟。none/mq-none：不排序不合并，直接发送，适合 NVMe（无寻道开销）。HFT NVMe 用 none 调度器减少延迟。

</details>

**Q2.** 为什么 SSD 不需要 IO 调度器？

<details><summary>答案</summary>

机械盘需要排序是因为寻道（磁头移动）是毫秒级，排序减少寻道距离。SSD 随机访问延迟恒定（~100μs），排序无收益反而增加 CPU 开销。现代 NVMe 用 multi-queue（每 CPU 一个提交队列 + 每设备一个完成队列），完全绕过传统 IO 调度器。这就是 `nvme.io_queue` > 1 的意义。

**修正**：「完全绕过」不准确——绕过是**默认 none** 的结果，不是硬件强制。NVMe 上依然可以挂 mq-deadline / kyber / bfq，只是内核默认不挂（`elevator_get_default()` 在 `nr_hw_queues != 1` 时返回 NULL，见 14.5）。另外 none 也不是零调度，plug 攒批时的简单合并仍在。

</details>

**Q3.** NVMe 上 `none` 和 `kyber` 都常见，什么时候该放弃默认的 `none` 而选 `kyber`？

<details><summary>答案</summary>

**当设备队列深度过大导致尾延迟恶化时。**

`none` 完全不限流，队列能排多深就排多深。NVMe 的 SQ 深度常开到 1024，高负载下请求在设备内部排队，`await` 平均可能还行但 p99/p999 会明显恶化——因为排队延迟直接叠加到尾部的请求上。

`kyber` 的解法（源码注释原话）："Controls latency by throttling queue depths"。它给每个调度域设了深度上限（READ 256 / WRITE 128 / DISCARD 64 / OTHER 16），并用直方图统计完成延迟，**超标就收缩该域深度、好转后再放开**，形成闭环。延迟目标默认值是 READ 2ms / WRITE 10ms。

所以判断依据：

- 负载轻、队列排不满 → 两者无差别，用 `none`（零开销更小）
- 负载重、队列常排满、p99 明显差于 p50 → **换 `kyber`**，用少量吞吐换回尾延迟
- 需要读优先、且在意写饿死读 → 换 **`mq-deadline`**（读 500ms / 写 5s 超时）

注意别把 `nr_requests`（软件队列深度，见 14.5）和 kyber 的深度上限混为一谈：前者是 blk-mq 层的 tag 池上限，后者是调度器额外加的动态限流。

</details>

</details>
---
