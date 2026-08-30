# 03 — 延迟测量方法论

> **对应 Rosen:** 无（3.x 无系统的延迟测量实践）
> **内核源码路径:** `Documentation/core-api/timekeeping.rst`、`lib/vdso/`

## 文档概述

低延迟系统的第一原则是：**不能度量就不能优化**。
但"在代码里打两个时间戳相减"这件事本身有巨大陷阱——时钟选错、测量点污染被测路径、
只看均值不看分位数，都会让你得出完全错误的结论。

本笔记给出可直接套用的测量方法。**这是 `14-hft-engineering/ch09` 与
`projects/P10` 中 `docs/benchmark.md` 的方法基础。**

---

## 核心内容

### 一、测什么：先定义清楚口径

| 口径 | 定义 | HFT 相关性 |
|------|------|-----------|
| 处理延迟 | 收到包 → 发单决策完成 | 你能控制的部分 |
| 单向延迟（OWD） | NIC 收包 → 对端 NIC 发出 | 需要两端时钟同步（PTP） |
| tick-to-trade | 行情包最后一字节进网卡 → 订单第一字节出网卡 | **行业标准口径** |
| 队列延迟 | 进 ring → 被取走 | 定位消费瓶颈 |

**必须先声明口径**，否则数字没有意义。同一个系统，不同口径能差一个数量级。

---

### 二、时钟选择（最容易踩的坑）

| 时钟 | 精度 | 开销 | 特点 |
|------|------|------|------|
| `CLOCK_REALTIME` | ns | ~25 ns | 墙上时间，**NTP 会跳变**，绝不能测间隔 |
| `CLOCK_MONOTONIC` | ns | ~25 ns | 单调，但受 NTP 频率微调影响 |
| **`CLOCK_MONOTONIC_RAW`** | ns | ~25 ns | **单调且不受 NTP 调整，内部测量首选** |
| `CLOCK_TAI` | ns | ~25 ns | 含闰秒偏移，配合 PTP 用 |
| **`rdtsc` / `rdtscp`** | TSC cycles | ~5–10 ns | **最快**，需校验 TSC 稳定性 |
| `rte_rdtsc()`（DPDK） | TSC cycles | ~5 ns | 同上，配 `rte_get_tsc_hz()` 换算 |

```bash
# 用 TSC 前必须确认这两个 flag，否则多核/变频下 TSC 会漂
grep -oE 'constant_tsc|nonstop_tsc|tsc_reliable' /proc/cpuinfo | sort -u
```

- `constant_tsc`：TSC 频率恒定，不受 CPU 变频影响
- `nonstop_tsc`：TSC 在 C-state 休眠时也不停
- 现代 x86 服务器基本都有；**虚拟机里必须显式校验**

**规则：** 内部微基准用 TSC（最快、最一致）；跨机对比用 PTP 同步后的 `CLOCK_TAI`。

**跨机同步：**
```bash
ptp4l -i eth0 -m -H          # 硬件时间戳 PTP（优先）
phc2sys -a -r -m             # 把网卡 PHC 同步到系统时钟
# 或用软件 PTP，精度从 ~100ns 级降到 ~数十 μs 级，HFT 不够用
```
要亚微秒精度必须网卡支持**硬件时间戳**（`ethtool -T eth0` 确认）。

---

### 三、分位数，不是均值

**均值是最没用的指标。** 决定策略生死的是尾延迟——
一次 GC 停顿、一次 TLB miss 风暴、一次网络抖动，够被逆向选择吃掉一整天的利润。

| 指标 | 含义 | 为什么重要 |
|------|------|-----------|
| p50 | 中位数 | 只反映常态 |
| **p99** | 1% 的请求比它慢 | 基本盘 |
| **p999** | 0.1% 比它慢 | **HFT 的真正考核线** |
| p9999 | 万分之一 | 定位极端抖动 |
| max | 最坏值 | 排查用，样本多时无统计意义 |

**记录方式：直方图，不是数组+排序。**

排序法要 O(n log n) 且内存无限增长；直方图 O(1) 记录、内存固定、随时可读。

```c
/* 简化版分位直方图：线性桶 + 溢出桶，O(1) 记录 */
#define HIST_BUCKETS      4096
#define HIST_NS_PER_BUCKET 64      /* 桶宽 64ns → 覆盖 0 ~ 262μs */

struct hist {
    uint64_t buckets[HIST_BUCKETS];
    uint64_t overflow;
    uint64_t count;
};

static inline void hist_record(struct hist *h, uint64_t ns)
{
    uint64_t idx = ns / HIST_NS_PER_BUCKET;
    if (idx < HIST_BUCKETS) h->buckets[idx]++;
    else                    h->overflow++;
    h->count++;
}

/* 分位：从最慢的桶往回累加，累计达到"应有 need 个样本比它慢"时的桶上界即为分位值 */
static uint64_t hist_quantile(const struct hist *h, double q)
{
    if (h->count == 0) return 0;

    /* ★ need 必须向上取整，直接截断会出两个错：
       · 溢出样本占比略低于 (1-q) 时，p999 被误判为"落在溢出区"；
       · 小样本时会把 max 当成 p999 —— 例：1001 个样本里 1 个极慢值，
         截断版返回那个极慢值，正确值应是第二慢的。尾延迟被系统性高估。
       这里手写取整而非 ceil()，是为了不依赖 libm。 */
    double   rank  = (double)h->count * (1.0 - q);
    uint64_t trunc = (uint64_t)rank;
    uint64_t need  = (rank > (double)trunc) ? trunc + 1 : trunc;
    if (need == 0) need = 1;

    uint64_t acc = h->overflow;
    if (acc >= need) return UINT64_MAX;          /* 分位落在溢出区，需加宽 */

    for (int i = HIST_BUCKETS - 1; i >= 0; i--) {
        acc += h->buckets[i];
        if (acc >= need)
            return (uint64_t)(i + 1) * HIST_NS_PER_BUCKET;
    }
    return 0;
}
```

**取舍：** 线性桶精度受桶宽限制（±64 ns），对 μs 级测量够用。
要跨 ns~s 多量级且保持相对精度，用 **HdrHistogram**（对数分桶，工业标准）。

---

### 四、误差来源清单（照着排除）

| 误差源 | 现象 | 消除方法 |
|--------|------|---------|
| **首次触碰** | 前若干次测量异常慢 | 预热：丢弃前 10% 样本 |
| **CPU 变频** | 结果随负载漂移，Turbo 开关影响 | `cpupower frequency-set -g performance`；关 Turbo/C-state |
| **上下文切换** | p999 突刺 | `isolcpus` + `taskset` 绑核 |
| **中断打进测量核** | 周期性突刺 | 队列中断绑别的核 + `managed_irq` |
| **SMT 争抢** | 兄弟核跑别的任务就变慢 | 关超线程，或独占一个物理核 |
| **NUMA 跨节点** | 平均慢 +100ns 且方差大 | `numactl --membind` 绑到网卡所在节点 |
| **编译器优化掉测量** | 时间戳被重排/消除 | `asm volatile("" ::: "memory")` 屏障 |
| **测量本身太重** | 测出的数包含测量成本 | 用 `rdtsc`；测空循环基线并扣除 |
| **NTP 跳变** | 出现负延迟 | 用 `CLOCK_MONOTONIC_RAW` |

```bash
# 一次性配齐测量环境
cpupower frequency-set -g performance
echo 1 > /sys/devices/system/cpu/intel_pstate/no_turbo   # Intel
sysctl -w kernel.perf_event_max_sample_rate=1
# 内核启动参数：isolcpus=domain,managed_irq,2-7 nohz_full=2-7 rcu_nocbs=2-7
```

**必须扣基线：** 先测一个空循环的时间戳开销，再从真实测量里减掉。
`clock_gettime()` 两次调用 ~50ns，在 μs 级测量里不是可忽略项。

---

### 五、报告模板（benchmark.md 该长什么样）

最小可用报告必须包含这四块，缺一块数字就不可信：

```markdown
## 环境
- CPU: <型号> @ 锁频 <GHz>，关闭 Turbo/C-state/超线程
- 内核: <版本>  |  NIC: <型号> + 驱动 <版本>
- 绑核: 队列 2 → CPU 3，进程 taskset -c 3
- 时钟: CLOCK_MONOTONIC_RAW / rdtsc（标明 TSC 稳定性）

## 口径
tick-to-trade：行情包最后一字节进 NIC → 订单首字节出 NIC

## 数据
样本数: 10,000,000   丢弃预热: 前 100 万
| p50 | p99 | p999 | p9999 | max |
|-----|-----|------|-------|-----|
| 1.2μs | 2.8μs | 7.4μs | 21μs | 1.3ms |

## 复现步骤
<完整命令，含 sysctl / ethtool / taskset>
```

**样本量硬要求：** p999 至少要 100 万样本才有参考价值，p9999 要 1000 万。
样本不足时分位数只是噪声。

---

## HFT 要点

- **看 p999，不看均值。** 均值掩盖了所有会让你亏钱的突刺
- 内部测量用 `CLOCK_MONOTONIC_RAW` 或 `rdtsc`；**绝不用 `CLOCK_REALTIME`**（NTP 会跳）
- 用 TSC 前先 `grep constant_tsc /proc/cpuinfo`，虚拟机里尤其要验
- **测量开销要扣基线**，两次 `rdtsc` 之间本身就有 ~5-10ns
- 每次报告都要写明环境，否则三个月后你自己都复现不出来
- 样本 < 100 万时的 p999 没有统计意义

→ 后续落地点：`projects/P10-hft-prototype/docs/benchmark.md`
→ 理论见 `14-hft-engineering/chapter-09-latency-measurement-benchmarking/`

## 与 Rosen 3.x 的差异

| 维度 | Rosen（3.x） | 现代（5.x/6.x） |
|------|-------------|----------------|
| 测量工具 | `jiffies`、`gettimeofday` | vDSO `clock_gettime`、rdtsc、PTP 硬件时间戳 |
| 精度基线 | 毫秒 / 十微秒 | 纳秒级，TSC 单次 ~5ns |
| 分位观念 | 未强调 | p999 是 HFT 硬指标，HdrHistogram 是标准做法 |
| 变频 | C-state/Turbo 影响小 | 现代省电激进，不锁频数据完全不可信 |
