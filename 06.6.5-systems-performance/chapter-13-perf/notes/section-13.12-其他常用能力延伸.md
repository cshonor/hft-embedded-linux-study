# 13.12 其他常用能力延伸

> [章节导航](../README.md) · 上一节：[13.11 perf trace](./section-13.11-perf-trace-系统调用追踪.md) · 下一章：[Ch 14 Ftrace](../../chapter-14-ftrace/README.md)

## 本节讲什么

perf 家族里四个专项子命令——`sched` / `lock` / `c2c` / `mem` / `annotate`。它们不是日常工具，但每个都对应一类 HFT 特有病灶，且**触发条件明确**：知道症状 → 知道用哪个 → 知道输出怎么读。

---

## 1. `perf sched` — 调度行为分析

```bash
perf sched record -p $(pidof strategy) -- sleep 10   # 采 sched tracepoint 流
perf sched latency                                    # 每线程的调度延迟表
perf sched timehist                                   # 时间线视图
```

### 1.1 `perf sched latency` 输出精读

```
  Individual delays for each thread (in ms):

  task               CPU   run time  delay      avg delay    max
  ---------------   ----  ---------  --------  ----------  -------
  strategy:4242      [03]   942.3 ms    5.1 ms   0.012 ms   3.854 ms
  strategy:4243      [05]    88.1 ms   21.9 ms   0.431 ms   18.722 ms
```

| 列 | 含义 | HFT 判读 |
|---|---|---|
| delay | **就绪→上 CPU 的总等待** | 唤醒延迟的直接证据 |
| avg / max delay | 平均/最坏唤醒等待 | max 大 = 有过被低优先级/中断压住的瞬间 |
| run time | 实际占 CPU | 与 delay 的比例即"调度健康度" |

**与 BPF runqlat 的分工**：`perf sched latency` 是**逐线程表**（谁等得多）；runqlat 是**全系统直方图**（P50/P99 分布形态）。诊断"线程 4243 为什么等"用前者，监控"整机调度恶化了吗"用后者（[ch16.1.7](../../chapter-16-case-studies/notes/section-16.1.7-16.1.8-动态追踪与结论.md)）。

## 2. `perf lock` — 锁竞争

```bash
perf lock record -p $(pidof strategy) -- sleep 10
perf lock report            # 按锁聚合：争用等待直方
perf lock report -F held    # 按持有时间
```

数据源是内核 lockdep/lock tracepoint 事件（`lock:lock_acquire/lock_contended`）——要求内核配置开启。输出按**锁实例**聚合：哪把锁、争用了多少次、等了多久。

| 场景 | 读法 |
|---|---|
| 高频 contended + 短等待 | 自旋态争用（轻）——无竞争 fastpath 已挡不住，考虑细粒度/无锁结构 |
| 低频 contended + **长等待** | 真持锁过久——找持锁方（配合火焰图） |
| futex 调用多但 lock 无争用 | 用户态锁（pthread 语义），内核 lock 事件看不见——转 [perf trace futex 画像](./section-13.11-perf-trace-系统调用追踪.md) |

> 限制：perf lock 只看**内核可见的锁事件**；纯用户态 futex 的争用细节要 BPF（futex tracepoint / offcputime 栈）。

## 3. `perf c2c` — 伪共享 / cache line 争用 ⭐

`c2c` = **cache-to-cache**。专抓**跨核缓存一致性流量**：

```bash
perf c2c record -p $(pidof strategy) -- sleep 15
perf c2c report
```

**机制**：采样 LLC load 事件时记录**数据虚拟地址 + 物理地址 + 命中的 cache line 所在核**——如果多个核反复 hit/modify 同一 cache line（各改各的偏移），报告直接点名**地址、偏移、涉事核**。

**HFT 杀手场景——false sharing**：

```c
struct counters {          // 两个线程各写各的字段，看似无竞争
    uint64_t ticks;        // 线程 A 写（0x00）
    uint64_t msgs;         // 线程 B 写（0x08）——同一 64B cache line！
};
// MESI：每次写都让对方核的 line 失效 → 缓存行乒乓 → 每次写变跨核总线事务
```

| 报告要素 | 读法 |
|---|---|
| HITM（Hit Modified）率 | 核 A 读到核 B 改过的 line——乒乓证据 |
| 涉事地址列表 | 直接对回源码结构体字段 |
| 核对（origin ↔ target） | 谁和谁在乒乓 |

**修法**：字段按核分 cacheline 对齐（`alignas(64)`）、per-CPU 计数（与内核 pcp/SLUB 同思想，[06-linux-mm](../../../06-linux-mm/)）、热字段分结构。

**适用条件**：需 CPU 支持 load latency 采样（Intel Haswell+ 的 LDLAT 事件；ARM 平台支持有限——Pi5 上 c2c 基本不可用，靠 [Ch 6 PMC](../../chapter-06-cpus/) 手工观测）。

## 4. `perf mem` — 内存访问剖析

```bash
perf mem record -p $(pidof strategy) -- sleep 10
perf mem report            # 按 数据地址/指令 聚合 load 延迟
```

采样 load/store 的**数据地址 + 延迟 + 本地/远端**（NUMA 维度）。与 c2c 的分工：mem 看"**哪个访问慢**"（NUMA 远端、TLB miss），c2c 看"**哪条 line 在乒乓**"。同样依赖平台 load-latency 采样支持。

## 5. `perf annotate` — 指令级热点

```bash
perf record -F 99 -e cycles -p PID -- sleep 30
perf annotate decode_tick        # 该函数逐指令的样本占比 + 源码对照
```

热点函数内部下钻：**哪一行/哪条指令**烧的周期。配合 `:pp`（[precise_ip，13.3](./section-13.3-13.7-perf-事件源.md)）指令归因更准。优化 order book 数据结构时：annotate 告诉你慢在 miss 的那次访问还是分支预测失败的那条跳转。

## 6. 全家族选型表（章末收束）

| 症状 | 工具 | 生产可用 |
|---|---|---|
| 整机/进程计数画像 | `perf stat`（13.8） | ✅ 常驻 |
| CPU 热点 + 调用链 | `record -g` + 火焰图（13.9/13.10） | ⚠️ 限窗 |
| syscall 行为画像 | `perf trace -s`（13.11） | ⚠️ 限窗 |
| 调度等待归因 | `perf sched latency` | ⚠️ 限窗 |
| 锁争用分账 | `perf lock report` | ⚠️ 限窗 |
| **伪共享定位** | `perf c2c record` | 开发机 |
| NUMA/访问延迟 | `perf mem report` | 开发机 |
| 函数内指令热点 | `perf annotate` | 离线 |

与 BPF 的总分工（承 [Ch 15](../../chapter-15-bpf/)）：perf 家 = **预定义观测**（快、稳、零开发）；BPF 家 = **自定义观测**（任意谓词聚合）。成熟的低延迟团队两家并用：perf 做基线巡检，BPF 做专项深挖。

---

## HFT / 嵌入式关联

| 场景 | 用法 |
|---|---|
| 多线程计数器优化 | `perf c2c` 点名乒乓地址 → `alignas(64)`/per-CPU 计数——教科书级 ROI |
| NUMA 亲和审计 | `perf mem` 远端访问占比——网卡与内存的 socket 对齐（对照 [10 网络](../../chapter-10-network/)） |
| 唤醒延迟验收 | `perf sched latency` 前后对比——`isolcpus`/优先级调整的效果证据 |
| 热点函数微雕 | `perf annotate` + `:pp`——数据结构布局优化的最后一公里 |
| 弱平台（Pi5） | c2c/mem 依赖平台采样器常缺席——退回 Ch 6 PMC 手工事件组合 |

---

## 衔接

- 本章完 → [Ch 14 Ftrace](../../chapter-14-ftrace/README.md)（tracepoint 的另一消费面 + hwlat）
- BPF 专项深挖：[Ch 15](../../chapter-15-bpf/) · [06.7-bpf-observability](../../../06.7-bpf-observability/)
- 微架构背景：[Ch 6 CPU](../../chapter-06-cpus/) · [15-computer-architecture](../../../15-computer-architecture/)
- 方法论总装：[ch16 案例演练](../../chapter-16-case-studies/notes/section-16.9-HFT-版Unexplained-Win演练模板.md)

---

## 代码自测

<details><summary>Q1：false sharing 为什么会拖慢两个"各写各字段"的线程？</summary>

两个字段落在同一 64B cache line，缓存一致性按 line 粒度工作：核 A 写 ticks 使核 B 的该 line 失效，核 B 写 msgs 又使 A 失效——每次写都触发跨核无效化/传输。数据层面无竞争，缓存层面全是竞争。
</details>

<details><summary>Q2：perf c2c 是怎么把"乒乓的 line"点出来的？</summary>

采样 LLC load 事件并记录数据物理地址 + 命中来源核。同一物理地址被多个核反复以"命中对方修改过的 line"（HITM）访问 → 该地址进入报告：涉事偏移、核对、频率。修法是让各核的热字段分属不同 line。
</details>

<details><summary>Q3：perf sched latency 和 runqlat 各回答什么问题？</summary>

sched latency 给**逐线程表**（哪个线程等得多、avg/max），runqlat 给**全系统直方图**（P50/P99 分布形态）。定位"谁受害"用前者，监控"整机是否恶化/双峰"用后者。
</details>

<details><summary>Q4：perf lock 报告无争用但 futex 调用极多，矛盾吗？</summary>

不矛盾：perf lock 消费的是内核 lock tracepoint（主要覆盖内核锁）；pthread 互斥的争用走 futex syscall 路径，内核 lock 事件看不到。用户态锁画像用 perf trace 的 futex 汇总或 BPF futex/uprobe 观测。
</details>

<details><summary>Q5：perf mem 和 perf c2c 的分工边界？</summary>

mem 回答"哪个**访问**慢"（NUMA 远端、TLB miss、延迟分布），c2c 回答"哪条 **cache line** 在多核间乒乓"。前者是延迟视角，后者是一致性流量视角；NUMA 优化用 mem，伪共享修整用 c2c。
</details>
