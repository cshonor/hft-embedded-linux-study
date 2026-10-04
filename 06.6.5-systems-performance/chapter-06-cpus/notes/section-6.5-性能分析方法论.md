## 6.5 性能分析方法论

> 章节导航：[本章导读](../README.md) · 下一篇 [6.6–6.7 观测工具与可视化](./section-6.6-6.7-观测工具与可视化.md)

**本节讲什么**：CPU 的 USE 检查（saturation 优先）、统计/剖析/周期分析三级方法、IPC 分解树与 Top-down 四分类的衔接、per-CPU 视角的必要性。

### 要点

| # | 要点 | 一句话 |
|---|------|--------|
| 1 | CPU 的 S（saturation）比 U 重要 | HFT 尖刺来自排队不是利用率 |
| 2 | 三级方法递进 | 统计（快）→ 剖析（归属）→ 周期（微观） |
| 3 | **per-CPU 视角**是 HFT 默认 | 全局平均掩盖热核饱和 |
| 4 | IPC 分解树导向**四类微观瓶颈** | cache/branch/frontend/backend |
| 5 | 优化前后**各跑一次 perf stat** | 比感觉靠谱 |

---

### 一、USE 方法（CPU）

对**每个 CPU**（或每组 dedicated cores）：

| 字母 | 问什么 | 怎么量 | 判读 |
|------|--------|--------|------|
| **U** Utilization | 非 idle % | `mpstat -P ALL 1` | 高 U 本身不是问题（算力被用是好事） |
| **S** Saturation | run queue、调度延迟 | `vmstat 1` 的 `r`；`runqlat` 直方图；**PSI cpu** | r > 核数持续 = 排队；PSI some > 0 = 有线程在等 CPU |
| **E** Errors | 硬件错 | `mcelog`、EDAC、RAS | 罕见但必须查 |

**为什么 S 比 U 重要**：U=90% 的系统可以完全健康（吞吐型），U=30% 的系统可以延迟糟糕（锁竞争 + 频繁上下文切换）——**延迟是排队现象，利用率只是背景**。HFT 专用核上 U 应该低（只跑策略），**任何调度延迟都是异常**。

> 完整检查表：[附录 A](../../appendix-A-USE方法Linux.md)

### 二、三级分析方法

```
第 1 级 统计（秒级，开销≈0）
  mpstat / vmstat / pidstat / PSI
  → 回答：忙不忙？排不排队？谁在忙？

第 2 级 剖析（分钟级，低开销）
  perf record -g / BCC profile / 火焰图
  → 回答：CPU 时间花在哪些栈上？（on-CPU 归属）

第 3 级 周期分析（PMC，微观）
  perf stat / perf record -e 事件
  → 回答：cycles 花在哪类微操作上？（cache miss？分支？）
```

三级是**漏斗**：先用统计排除（CPU 根本不忙 → 去 ch10/ch9），再剖析归属（热点栈），最后对热点做周期分析（为什么这段慢）——跳级（上来就 perf stat）常见浪费。

**剖析（Profiling）要点**：定时采样（固定频率中断 → 采 PC+栈 → 统计栈频次）——采样频率与时长足够；**热路径要有符号 + 帧指针**（`-fno-omit-frame-pointer`，[ch5 Gotchas](../../chapter-05-applications/)）。

**Off-CPU 补充**：剖析只看 on-CPU——「等锁/等 I/O」的时间在火焰图里**不可见**，要 offcputime（[ch5](../../chapter-05-applications/)、[ch15](../../chapter-15-bpf/)）补另一半。

### 三、周期分析（Cycle Analysis）

从 **IPC** 出发（[ch16.1.5 PMC 判读](../../chapter-16-case-studies/)、事件来源见 [ch13 perf stat](../../chapter-13-perf/notes/section-13.8-perf-stat-事件计数.md)）：

```
高 cycles + 低 IPC（每周期做的指令少）
  ├── cache miss 高（LLC-load-misses / HITM）
  │     → 数据结构布局 / 对齐 / NUMA 远端（Ch 7 + [06-linux-mm](../../../06-linux-mm/)）
  ├── branch miss 高（branch-misses）
  │     → 不可预测分支：查表替代 if、位技巧
  ├── frontend stall（L1-icache-load-misses、idq 空）
  │     → 代码膨胀、I-cache 冷、指令解码瓶颈
  └── backend stall（资源冲突、依赖链）
        → 数据依赖串行、执行端口竞争
```

**Top-down 四分类**（[ch13.12](../../chapter-13-perf/notes/section-13.12-其他常用能力延伸.md)）把这棵树自动化：Retiring（好）/ Bad Speculation（分支错）/ Frontend Bound / Backend Bound——一级归类直接指向优化方向。

**HFT**：优化 order book 数据结构前后各跑一次 `perf stat`，对比 IPC 与 `LLC-load-misses`——数据结构改动对 cache 的影响有客观数字，比凭感觉改靠谱。

### 四、per-CPU 视角

HFT 热路径绑特定核——**全局平均是误导源**：

```
全局 mpstat：all  12.0% usr          ← 「很闲」
per-CPU：    CPU2  97.5% usr          ← 热核已饱和
            CPU3   0.0%               ← 隔离核（正常）
            CPU7  45.2%               ← softirq 核（网卡）
```

判读纪律：**热核的 r 队列、上下文切换、%soft 分开看**；`mpstat -P ALL 1` 而不是裸 `mpstat 1`。上下文切换率（`vmstat 1` 的 cs）在热核上应接近常数——突增 = 有东西在抢占。

### 五、60 秒 CPU 检查

```bash
uptime                      # load 相对核数
mpstat -P ALL 1             # per-CPU 使用率 + %soft
vmstat 1 5                  # r 队列、cs 上下文切换
pidstat -t 1 3              # 线程级归属
cat /proc/pressure/cpu      # PSI stall
perf stat -a sleep 5        # 全系统 IPC / 频率 / 迁移
```

### 衔接

- 下一节：[6.6–6.7 观测工具与可视化](./section-6.6-6.7-观测工具与可视化.md)
- 关联：[ch2 USE/Off-CPU](../../chapter-02-methodologies/)、[ch13 perf](../../chapter-13-perf/)（周期分析的武器）、[ch16 PMC 判读](../../chapter-16-case-studies/)、[06-linux-mm](../../../06-linux-mm/)（cache/TLB/NUMA 的内核机制）

---

### 常见陷阱

1. **USE 只查 Utilization**——saturation（run queue/调度延迟/PSI）才是 HFT 关键。
2. **profiling 不加帧指针**——`-g` 栈回溯全是 [unknown]。
3. **只看全局 CPU**——热核饱和被平均稀释；`mpstat -P ALL`。
4. **跳级分析**——不先统计排除就 perf stat，浪费在错误方向。
5. **忘了 on-CPU 剖析的盲区**——等锁等 I/O 的时间不在火焰图里，offcputime 补。

<details>
<summary>自测题（点击展开）</summary>

1. CPU 的 USE 方法中 HFT 最该关注哪个字母？
   <details><summary>答</summary>Saturation——run queue 长度和调度延迟（runqlat/PSI）；HFT 尖刺多因排队而非 CPU 不够。</details>
2. 三级分析方法的递进逻辑？
   <details><summary>答</summary>统计（秒级排除/定位方向）→ 剖析（on-CPU 时间归属）→ 周期分析（cycles 微观去向）——漏斗式缩小范围。</details>
3. IPC 低 + LLC-load-misses 高，指向什么？
   <details><summary>答</summary>backend 数据供给不足——数据结构布局/对齐/NUMA 远端访问问题（Ch7 + 06-linux-mm）。</details>
4. 为什么火焰图看不到锁等待？
   <details><summary>答</summary>剖析采样的是「在 CPU 上跑的栈」——等锁时不在 CPU 上，采样抓不到；要 offcputime 看 off-CPU 栈。</details>
5. 全局 12% 使用率却延迟尖刺，先查什么？
   <details><summary>答</summary>mpstat -P ALL 找热核 + runqlat 看调度延迟——大概率是某核饱和或隔离失效（其它核闲着）。</details>

</details>


---

← [本章导读](../README.md)
