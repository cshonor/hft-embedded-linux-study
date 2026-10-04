# 13.3–13.7 perf 事件源

> [章节导航](../README.md) · 上一节：[13.1–13.2 子命令概述](./section-13.1-13.2-子命令概述与单行命令.md) · 下一节：[13.8 perf stat](./section-13.8-perf-stat-事件计数.md)

## 本节讲什么

perf 的四类事件源（Hardware / Software / Tracepoint / Probe）。重点不是背事件名——而是分清它们在内核里走的**三条实现路径**，以及两个决定数据质量的关键机制：

1. **PMC 计数器只有 4~8 个** → 事件超配时的 **multiplexing 轮换**（perf stat 数字里的 `scale` 之谜）
2. **采样地址的 skid 滑差** → `precise_ip` 字段与 PEBS/SPE

---

## 1. 四类事件源总览

| 类型 | `perf list` 前缀 | 实现路径 | 计数发生在哪 |
|---|---|---|---|
| **Hardware（PMC）** | 无前缀（cycles…） | x86_pmu / ARMv8 PMU 驱动 | **硬件计数器** |
| **Software** | `software:`（可省） | core.c 内核计数（`perf_swevent` PMU，:10018） | 内核事件点 |
| **Tracepoint** | `sched:` `block:` `syscalls:` … | 挂 Ftrace tracepoint（[Ch 14](../../chapter-14-ftrace/notes/section-14.5-14.714.10-事件源Filter-与-Hist-Triggers.md)） | 内核静态埋点 |
| **Probe** | `probe:*`（kprobe/uprobe/USDT） | 动态插桩（perf probe 创建） | 断点陷阱 |

**核心认知**：perf 没有自己的插桩基础设施——tracepoint/kprobe/uprobe 全部**复用 Ftrace 体系**（经由 `struct pmu` 的 perf_tracepoint/perf_probe 实例桥接）。perf 与 Ftrace 是**同一套观测点的两种消费方式**（perf 用 fd + ring buffer，Ftrace 用 tracefs 文件）。

## 2. Hardware 事件（PMC）

### 2.1 常用事件与判读去向

| 事件 | 含义 | 判读 |
|---|---|---|
| `cycles` | CPU 周期 | IPC 分母 |
| `instructions` | retired 指令 | IPC 分子 |
| `cache-references/misses` | LLC 级 cache 行为 | 内存墙 |
| `L1-dcache-load-misses` | L1D miss | 数据布局 |
| `branch-misses` | 分支预测失败 | 热分支 |
| `stalled-cycles-frontend/backend` | 流水线停顿（x86 老事件） | 新 CPU 用 Top-down 替代 |

判读方法（IPC 四象限、Top-down）在 [ch16.1.5 PMC 与软件事件精读](../../chapter-16-case-studies/notes/section-16.1.5-16.1.6-PMC-与软件事件.md)；微架构背景在 [Ch 6 CPU](../../chapter-06-cpus/)。

### 2.2 ⭐ multiplexing：计数器不够用时的轮换

**问题**：每核通用 PMC 通常只有 **4~8 个**（x86 常见 8，ARMv8 常见 6）。`perf stat -e` 列 12 个硬件事件时放不下。

**内核解法**（v6.6 锚点）：

| 部件 | 说明 | 位置 |
|---|---|---|
| 每 CPU 一个 hrtimer | `__perf_mux_hrtimer_init()`，默认间隔 `PERF_CPU_HRTIMER = 1000/HZ` ms（一个 tick） | core.c:1066/:1090 |
| 到期回调 | `perf_mux_hrtimer_handler()` → `perf_rotate_context()`——把放不下的事件**分组轮换**上下 CPU | core.c:1070 |
| 结果 | 每个事件只有部分时间在计数 → perf stat 读出**原始计数 + 计数时间占比 `time_enabled/time_running`** | — |

这就是 perf stat 输出里 `(xx.xx%)` 的含义，perf 用 `count × time_enabled / time_running` **外推**（scale）满时段值。**代价**：外推有误差，两组轮换的事件之间相关性也被破坏（无法比较同一瞬间的 cache-miss 与 branch-miss）。

**工程对策**：

| 手段 | 说明 |
|---|---|
| `-e` 少放事件 | 一轮 ≤ 4~6 个硬件事件，跑两轮 |
| `--no-multiplex` | 关轮换——放不下直接报错，宁可显式失败 |
| 事件分组 | `{}:u` 分组内互斥计数轮换，组间并行 |

### 2.3 ⭐ skid 与 precise_ip

PMC 溢出中断到达时，流水线已又执行了若干条指令——**采样到的 IP 不是溢出瞬间的指令**，这段滑差叫 **skid**。`perf_event_attr.precise_ip`（uapi perf_event.h:426–435）声明容忍度：

| 值 | 语义 |
|---|---|
| 0 | 任意 skid |
| 1 | 恒定 skid |
| 2 | 请求 0 skid |
| 3 | **必须** 0 skid（x86 PEBS / ARM SPE） |

x86 上 PEBS（Processor Event-Based Sampling）把溢出时的寄存器组快照进内存，几乎消除 skid；ARMv8 有 SPE（Statistical Profiling Extension，Pi5 的 Cortex-A76 无，Graviton/新 Neoverse 有）。**HFT 视角**：追"哪条指令导致 LLC miss"必须 `:p` 以上（perf 自动加 `:pp` 尽力精确），否则热点归因差好几条指令。

## 3. Software 事件

内核事件点直接计数，**不占 PMC**（v6.6：软件事件分发 `do_perf_sw_event()` core.c:9766，`___perf_sw_event()` :9805，CPU_CLOCK 处理 :9990）：

| 事件 | 触发点 |
|---|---|
| `page-faults` / `minor/major` | 缺页异常路径（→ [06-linux-mm fault 四路分发](../../../06-linux-mm/)） |
| `context-switches` | `__schedule()` |
| `cpu-migrations` | 任务迁核 |
| `cpu-clock` / `task-clock` | 定时器驱动的时间计数 |
| `emulation-faults` | 指令模拟 |

软件事件**永远可用**（无硬件依赖）但本身有内核成本（每次触发走一次事件路径）——`page-faults` 高频进程上 stat 无妨，**record 采样软件事件**时注意开销。

## 4. Tracepoint 事件

```
perf list 'sched:*'
perf record -e sched:sched_switch -a -- sleep 5
perf stat -e 'syscalls:sys_enter_read' -p PID -- sleep 3
```

tracepoint 字段与 Ftrace 完全同一套（`events/…/format`）——[Ch 14.5 事件源](../../chapter-14-ftrace/notes/section-14.5-14.714.10-事件源Filter-与-Hist-Triggers.md)讲过 ABI 稳定性。perf 消费方式的独特价值：

| 消费方式 | Ftrace | perf |
|---|---|---|
| 计数 | hist trigger | ✅ `perf stat -e tracepoint`（当计数器用） |
| 采样 | — | ✅ **按 tracepoint 触发采调用栈**（如每次 major-fault 采一次栈） |
| 时间线 | trace_pipe | perf script |

**杀手用法**：`perf record -e major-faults -g`——**每次缺页采一个栈**，直接看"谁在触发缺页"（对照 [Ch 7 缺页火焰图](../../chapter-07-memory/)）。

## 5. Probe 事件（kprobe / uprobe / USDT）

| 类型 | 插桩对象 | 创建命令 | 稳定性 |
|---|---|---|---|
| kprobe | 内核函数 | `perf probe -a 'tcp_v4_rcv skb'` | 随内核漂移 |
| uprobe | 用户函数偏移 | `perf probe -x ./strategy 'decode_tick'` | 随二进制漂移 |
| USDT | 应用静态埋点 | `perf probe -x ./strategy 'udt:tick_begin'` | 应用自控，稳 |

```bash
perf probe -x /path/strategy 'decode_entry'      # 创建 uprobe 事件
perf record -e probe_strategy:decode_entry -p PID -- sleep 10
perf probe -d 'decode_entry'                     # 用完删
```

**开销警示**：uprobe 每次命中都是一次 int3 陷阱 + 页权限操作——热路径函数（每 tick 调用的解码器）上显著增延迟。**开发/短采用**；生产要长时间观测热点区间 → 优先 99Hz 采样或 USDT（应用自己埋的点可以做成零成本未启用态）。

与 BPF 的分工：BPF kprobe/uprobe 走同一插桩点但回调在内核内执行（[Ch 15](../../chapter-15-bpf/)）——perf probe 每命中一次就把样本送用户态，BPF 可以在内核里先聚合。高频点 BPF 赢，低频点（如异常路径）perf probe 足够。

---

## HFT / 嵌入式关联

| 场景 | 组合 |
|---|---|
| order book 优化验收 | 前后各一轮 `perf stat -e cycles,instructions,LLC-load-misses,branch-misses`——IPC 与 LLC miss 的 delta 是硬证据 |
| 采样精确归因 | 事件加 `:pp`（precise_ip=2），PEBS/SPE 可用时热点对到指令 |
| 缺页归因 | `perf record -e major-faults -g` → 缺页火焰图（THP/mmap 决策的数据来源，对照 [06-linux-mm](../../../06-linux-mm/)） |
| 事件超配纪律 | 生产脚本固定 ≤6 个硬件事件/轮；对照分析跑两轮，不用 multiplexing 外推值下结论 |
| Pi5 | ARM PMU 事件名与 x86 有差异（`perf list` 勘探）；无 SPE → precise_ip 上限低，归因粒度粗 |

---

## 衔接

- 上一节：[perf 三层架构与 fd API](./section-13.1-13.2-子命令概述与单行命令.md)
- 下一节：[13.8 perf stat——计数流的完整机制](./section-13.8-perf-stat-事件计数.md)
- tracepoint 的另一消费面：[Ch 14 Ftrace](../../chapter-14-ftrace/README.md)
- 可编程对照：[Ch 15 BPF](../../chapter-15-bpf/)

---

## 代码自测

<details><summary>Q1：为什么 perf stat 输出里有的事件带 <code>(53.21%)</code>？</summary>

multiplexing：硬件 PMC 放不下所有事件，内核用 hrtimer（默认每 tick，core.c:1066/:1090）轮换事件组。括号是 `time_running/time_enabled` 占比，perf 按比例外推计数。外推值有误差且组间不可比。
</details>

<details><summary>Q2：perf 和 Ftrace 在 tracepoint 上的关系？</summary>

同一套插桩点、两种消费面。tracepoint 基础设施属于 Ftrace 体系（tracefs）；perf 经 `perf_event_open` 把 tracepoint 事件接进 fd + ring buffer 世界，可计数（stat）、可采样采栈（record -e tracepoint -g）、可时间线（script）。
</details>

<details><summary>Q3：skid 是什么？怎么消除？</summary>

PMC 溢出中断到达时流水线已继续执行多条指令，采到的 IP 偏离真正触发事件的那条——这段滑差即 skid。`attr.precise_ip` 声明容忍度（uapi:426–435）；x86 用 PEBS、ARM 用 SPE 硬件快照寄存器组实现 0-skid。perf 事件名加 `:p/:pp/:ppp` 请求。
</details>

<details><summary>Q4：软件事件和硬件事件"计数发生在哪"的区别，为什么软件事件不占 PMC？</summary>

硬件事件由 PMC 硬件自增（CPU 微码级），内核只在读/溢出时介入；软件事件在内核代码路径的埋点处经 `___perf_sw_event()`（core.c:9805）分发计数——纯内核内存操作，与 PMC 寄存器无关。代价是每次触发都有内核执行成本。
</details>

<details><summary>Q5：uprobe 插热路径函数为什么危险？有没有替代？</summary>

每次函数命中都是 int3 陷阱 + 单步恢复 + 可能的页表操作，高频函数上微秒级膨胀。替代：99Hz 采样看统计热点；USDT 由应用方控制埋点密度；BPF uprobe 有批量优化（内核内过滤后再送样本）。
</details>
