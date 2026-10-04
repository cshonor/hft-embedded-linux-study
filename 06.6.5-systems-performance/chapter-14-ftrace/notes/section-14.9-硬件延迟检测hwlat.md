# 14.9 硬件延迟检测（hwlat）

> [章节导航](../README.md) · 上一节：[14.5–14.7、14.10 事件源、Filter 与 Hist Triggers](./section-14.5-14.714.10-事件源Filter-与-Hist-Triggers.md) · 下一节：[14.11–14.13 前端工具](./section-14.11-14.13-前端工具.md)

## 本节讲什么

排查链的最后一块拼图：**连内核事件都看不见的停顿**。perf、offcputime、hist trigger 全都基于"OS 还在正常运行"这个前提——而 SMI（系统管理中断）发生的瞬间，**CPU 整体进入 SMM 模式，OS 被冻结、时钟读数停跳**，一切软件观测手段集体失明。hwlat 是用"测量本身的缺口"来暴露这类停顿的探测器。

---

## 1. 为什么 OS 级工具看不见 SMI

| 停顿类型 | OS 能看见吗 | 工具 |
|---|---|---|
| 等锁 / 等信号量 | 能 | offcputime、lockdep |
| 被抢占 / 等调度 | 能 | runqlat、sched tracepoint |
| 页错误 / I/O 等待 | 能 | biolatency、page-fault trace |
| **SMI / 固件窃取 / BMC 干扰** | **不能** | **hwlat** ← 本节 |

SMI（System Management Interrupt）触发后 CPU 切入 SMM 模式执行固件代码（厂商封闭、不可见），期间：

- OS 的所有核**冻结**——tracepoint 不会记录任何事件
- **TSC 继续走**（多数平台），但 OS 自己不在这段时间执行——事件序列出现"空洞"
- 典型来源：BIOS 的 ECC 刷洗、温度轮询、电源管理、IPMI/BMC 通信

**症状特征**（对照 [ch16 五类真相](../../chapter-16-case-studies/notes/section-16.0-案例背景An-Unexplained-Win.md)）：P99/P999 尖刺，但 perf CPU 剖析、offcputime、锁统计**全部对不上账**——CPU 显示非 idle，却没有任何 forward progress。

## 2. hwlat 原理：用采样窗口里的时间空洞当探测器

思路极其朴素：**一个线程死循环读时钟，如果两次读数间隙异常大，说明这段时间 CPU 被偷走了**。

v6.6 实现（kernel/trace/trace_hwlat.c）：

| 部件 | 实现 | v6.6 锚点 |
|---|---|---|
| 参数容器 | `struct hwlat_data`（window/width/mode/count） | :103 |
| 采样循环 | `get_sample()`：busy-loop 里反复读 clock，`do … while (total <= sample_width)` | :201，循环边界 :272 |
| 停顿判定 | 间隙 `> threshold` 才记录一条 `hwlat_sample` 事件 | :272 附近比较 |
| 内核线程 | `kthread_fn()` → 每 CPU 一个 `hwlatd/%u` | :360，`kthread_run_on_cpu` :499 |
| 轮换策略 | `move_to_next_cpu()`：默认 round-robin 逐核轮测 | :314 |
| 配置文件 | `hwlat_detector/{window,width,mode}` | :782 / :789 / :796 |

### 2.1 默认参数

| 参数 | 默认值 | 含义 |
|---|---|---|
| `window`（采样窗口） | **1 s**（:53 `DEFAULT_SAMPLE_WINDOW`） | 一个测量周期长度 |
| `width`（采样宽度） | **0.5 s**（:54 `DEFAULT_SAMPLE_WIDTH`） | 窗口内实际死循环采样的时长；剩余 0.5 s 线程睡眠让出 CPU |
| `mode` | `round-robin`（:117） | 每个窗口换一个 CPU 测；v6.6 另有 `per-cpu`（每核常驻一个 hwlatd）和 `none` |

> window > width 的设计让 hwlatd 自身**不占满 CPU**——采样线程本身也可能被调度出去，所以它测的是"该 CPU 上任何人都会遭遇的停顿"，而非自造负载。

### 2.2 阈值

阈值在 `tracing_threshold`（µs）。低于阈值的间隙不记录。经验设定：**目标 P99 的一半**（比如盯 10 µs 的尾延迟，阈值设 5）——太低会淹没在正常调度噪声里。

## 3. 用法

```bash
TR=/sys/kernel/tracing

echo 0 > $TR/tracing_on
echo hwlat > $TR/current_tracer
echo 5   > $TR/tracing_threshold          # µs
# 可选调参：
echo 1000000 > $TR/hwlat_detector/window  # 1 s（默认）
echo 500000  > $TR/hwlat_detector/width   # 0.5 s（默认）
echo per-cpu > $TR/hwlat_detector/mode    # 每核常驻（v6.6+）

echo 1 > $TR/tracing_on
sleep 60                                  # 低负载下采 1 分钟
echo 0 > $TR/tracing_on
cat $TR/trace
cat $TR/tracing_max_latency               # 本轮最大停顿
```

### 3.1 输出精读

```
# tracer: hwlat
#
#                              _-----=> irqs-off
#                             / _----=> need-resched
#                            |  / _---=> hardirq/softirq
#                            || / _--=> preempt-depth
#                            ||| /
#                            ||| /     delay
#           TASK-PID   CPU#  ||||    TIMESTAMP    FUNCTION
#              | |       |   ||||       |         |
           <...>-xxxxx  [003] d...   512.345678us: #475     inner=xx outer=yy
                                                              ^ nmi_total_ts=zz
```

| 字段 | 含义 |
|---|---|
| `#475` | 本次停顿序号（count，hwlat_data.count） |
| `inner` / `outer` | 采样循环**内层/外层**两次检测到的间隙（µs）——inner 更接近真实停顿时长 |
| `nmi_total_ts` | 若配了 NMI watchdog 辅助（`CONFIG_HWLAT_TRACER` + nmi 选项），**NMI 上下文里测到的间隙**——NMI 不受 SMM 影响，是 SMI 定罪的铁证 |

> **inner vs outer**：内层循环读表极密（无函数调用），outer 含少量循环维护——两者同时超标才更可信；只有 outer 大时可能是调度噪声。

### 3.2 与其他证据交叉

| 手段 | 看什么 |
|---|---|
| `cat /proc/interrupts` 两次采样差值 | SMI 行计数是否在涨（部分平台暴露 `SMI` 或 `LOC/SPU` 相关计数） |
| `turbostat`（x86） | `SMI` 列计数、C-state 驻留 |
| hwlat `mode=per-cpu` + `tracing_cpumask` 只留隔离核 | 确认停顿是否只发生在特定核（挨着 BMC 的 socket？） |
| BIOS 日志 / BMC SEL | 时间点对齐（ECC 刷洗、风扇策略事件） |

## 4. 对策清单（确诊后）

| 层 | 动作 |
|---|---|
| BIOS | 更新版本；关 `ECC Scrub` 周期性刷洗 / `Patrol Scrub`；`Power Management` → 最大性能；关 C-state 深睡（`idle=poll` 或 BIOS 级） |
| 内核启动参数 | `intel_idle.max_cstate=0 processor.max_cstate=0`（验代价：功耗↑）；`isolcpus` + `nohz_full` 隔离（→ [14-HFT ch05 调优联动](../../../14-hft-engineering/chapter-05-os-kernel-tuning/README.md)） |
| BMC/IPMI | 拉长轮询周期或改 event-driven；确认 BMC 与业务网口无共享中断 |
| 硬件 | 换主板/平台验证；PCIe 设备逐个拔除定位（固件缺陷设备的 PM 事件会触发 SMI） |

> **验收纪律**：裸机装机后、部署业务前，先跑 24h hwlat baseline（`mode=per-cpu` + 低阈值）——把"这台机器天生有 200 µs SMI"这类问题挡在上线前。对照 [ch16 baseline 纪律](../../chapter-16-case-studies/notes/section-16.0-案例背景An-Unexplained-Win.md)：先有基线，后谈异常。

---

## HFT / 嵌入式关联

| 场景 | 用法 |
|---|---|
| **尾延迟对不上账** | perf/offcputime/锁全部排除后，hwlat 是最后一环——`nmi_total_ts` 直接给 SMI 定罪 |
| 裸机选型验收 | 24h hwlat soak 是低延迟机器的**入场体检** |
| Pi5 / 嵌入式 | ARM 无 SMI 同款问题但有 PSCI/firmware 调用窃取——`mode=per-cpu` 逐核测同样有效 |
| 与隔离栈联动 | `nohz_full` + `isolcpus` 隔离做得再好，SMI 照样全核冻结——**固件层是隔离的天花板**，hwlat 量的是这个天花板 |
| 与 DPDK 关系 | DPDK busy-poll 核上 SMI 尤其致命（每秒百万次读时钟的轮询里空洞直接变丢包）——→ [13-dpdk](../../../13-dpdk/) |

---

## 衔接

- 上一节：[hist trigger 内核内聚合](./section-14.5-14.714.10-事件源Filter-与-Hist-Triggers.md)
- 下一节：[前端工具 trace-cmd / KernelShark / perf-tools](./section-14.11-14.13-前端工具.md)
- OS 层停顿排查链：[ch16.1.7 动态追踪与结论](../../chapter-16-case-studies/notes/section-16.1.7-16.1.8-动态追踪与结论.md) 的五类结局分流
- 调优动作：[14-HFT ch05 OS/内核调优](../../../14-hft-engineering/chapter-05-os-kernel-tuning/README.md)

---

## 代码自测

<details><summary>Q1：为什么 SMI 发生时 perf、tracepoint、BPF 全部失明？</summary>

三者都依赖"OS 代码在执行"。SMI 使 CPU 进入 SMM 模式执行厂商固件，OS 所有核冻结——没有事件可记。停顿只能从"事后时间戳的空洞"反推，这正是 hwlat 的原理。
</details>

<details><summary>Q2：hwlat 的采样循环为什么 window（1s）要大于 width（0.5s）？</summary>

width 是死循环读时钟的活跃时长，window 是完整周期。差额时间线程睡眠：既避免 hwlatd 自己成为 CPU 热点，也让"被测停顿"泛化到该 CPU 任意负载都会遭遇的水平。
</details>

<details><summary>Q3：hwlat 报了一个 300 µs 的停顿，怎么进一步确认是 SMI 而不是调度噪声？</summary>

① 看 `nmi_total_ts`——NMI 上下文的测量不受 SMM 冻结影响，有值即是铁证；② 对比 inner/outer（只有 outer 大偏调度噪声）；③ `mode=per-cpu` + 只留隔离核复测（隔离核无普通负载）；④ `/proc/interrupts` SMI 计数或 turbostat SMI 列对时间求差。
</details>

<details><summary>Q4：`mode=round-robin` 和 `mode=per-cpu` 各适合什么阶段？</summary>

round-robin（默认）：一个窗口换一个核，**普查**阶段用——低开销扫全部核。per-cpu（v6.6+）：每核常驻 hwlatd，**定位/监控**阶段用——抓间歇性停顿不漏窗，但常驻负载更高。
</details>

<details><summary>Q5：isolate + nohz_full 把核隔离好了，还需要 hwlat 吗？</summary>

需要。隔离解决的是"OS 层打扰"（调度/中断/timer），SMI 是**固件层打扰**——它冻结整个 CPU 包括你的隔离核。hwlat 量的正是隔离栈的天花板。
</details>
