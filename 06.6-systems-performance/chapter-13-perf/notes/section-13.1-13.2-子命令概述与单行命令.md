# 13.1–13.2 子命令概述与单行命令

> [章节导航](../README.md) · 下一节：[13.3–13.7 perf 事件源](./section-13.3-13.7-perf-事件源.md)

## 本节讲什么

perf 的**子命令地图**与 HFT 常备单行命令。但更重要的是先立起一个架构认知：`perf` 命令只是**用户态前端**——它的全部超能力来自内核的 `perf_event_open()` 系统调用和 PMU 硬件。看不懂这三层，后面所有机制（采样、multiplexing、ring buffer）都是黑盒。

---

## 1. perf 三层架构

```
┌─────────────────────────────────────────────┐
│ 用户态：perf(1) 命令行工具                    │
│   stat / record / report / script / trace…  │  ← tools/perf/*.c
│   只做三件事：配置 attr、读写数据、格式化输出   │
├─────────────────────────────────────────────┤
│ 系统调用边界：perf_event_open(attr, …)       │
│   v6.6: kernel/events/core.c:12351          │
│   返回一个 fd——"一个事件 = 一个 fd"          │
├─────────────────────────────────────────────┤
│ 内核：perf_event 核心 + 各 PMU 驱动           │
│   struct pmu 抽象（perf_event.h:302）        │
│   x86: arch/x86/events/*.c  ARM: drivers/perf│
│   软件事件在 core.c 内部计数                  │
├─────────────────────────────────────────────┤
│ 硬件：PMC 性能监控计数器（每核 4~8 个通用）     │
└─────────────────────────────────────────────┘
```

三个要点：

1. **一个事件 = 一个 fd**。`perf stat -e cycles,instructions` 背后是两次 `perf_event_open`，得到两个 fd；`read(fd)` 读累计计数、`mmap(fd)` 拿采样 buffer、`ioctl(fd, PERF_EVENT_IOC_ENABLE/…)` 控制启停（v6.6 core.c:5833–5843）。perf 子命令全是这套 fd API 的封装。
2. **PMU 是内核里的插件接口**。`struct pmu`（include/linux/perf_event.h:302）定义 `add/del/start/stop/read` 回调——x86 PMC、ARM PMU、软件事件（`perf_swevent`，core.c:10018）、tracepoint 各注册一个实例。**Pi5 上 perf 能用，就是因为 drivers/perf 下有 ARMv8 PMU 驱动**。
3. **版本为什么必须匹配**：perf_event_attr 结构和事件语义随内核演进（perf_event_open 是 v2.6.32 引入，此后每个版本都在加字段）；perf 工具检查 `perf_event_open` 返回的版本与自身编译头是否兼容——`linux-tools-$(uname -r)` 就是为了对齐这条边界。错配的典型症状：事件列表空、采样数据字段错位。

## 2. 子命令地图（按数据流组织）

```
                    ┌─ stat ──── read(fd) 累计计数（不采样）
                    ├─ top ──── 周期性 read + TUI
perf_event_open ────┼─ record ── mmap ring buffer 采样 → perf.data
        (fd)        ├─ report ─┐
                    ├─ script ─┤ 读 perf.data 离线分析
                    ├─ annotate┘
                    ├─ trace ── tracepoint 事件流（syscall 视图）
                    ├─ list ─── 列出可用事件（探测 PMU/tracepoint 覆盖面）
                    ├─ probe ── 创建 kprobe/uprobe/USDT 事件
                    └─ mem/sched/lock/c2c ─ 专项子命令（13.12）
```

**记忆锚点**：stat/top 是"计数流"（read 路径），record/report/script 是"采样流"（mmap 路径），trace 是"事件流"（tracepoint 路径）。三条数据流后面三节分别拆开。

## 3. HFT 常备单行命令

```bash
# --- 健康速查（计数流，开销≈0，生产可跑）---
perf stat -e cycles,instructions,cache-misses,branch-misses -- sleep 1
perf stat -p $(pidof strategy) -- sleep 5                  # 目标进程
perf stat -e cycles,instructions,page-faults,major-faults -p $(pidof strategy) -- sleep 10

# --- CPU 热点（采样流，限 PID + 限时长）---
perf record -F 99 -g -p $(pidof strategy) -- sleep 30
perf report --stdio --no-children | head -40

# --- 火焰图管道（需 FlameGraph 仓库）---
perf script | stackcollapse-perf.pl | flamegraph.pl > strategy.svg

# --- 实时 top（开发机）---
perf top -p $(pidof strategy)

# --- syscall 追踪（开发/debug，限时长）---
perf trace -p $(pidof strategy) -- sleep 5

# --- 事件勘探 ---
perf list | grep -E 'cache|fault|sched'     # 看 PMU 覆盖面
perf list pmu                               # 列出本机 PMU
```

**生产纪律**（每条命令背后都是真金白银的开销）：

| 命令 | 开销量级 | 生产可用性 |
|---|---|---|
| `perf stat` | ≈0（读几次计数器） | ✅ 长跑 |
| `perf top` | 低（周期 read） | ✅ 观察 |
| `perf record -F 99` | 每样本 µs 级 × 99/s | ⚠️ 限 PID+时长 |
| `perf trace` | 每 syscall 一条事件 | ⚠️ 限时长 |
| `perf probe` 热函数 | 每次调用一次 uprobe 陷阱 | ❌ 开发机 |

## 4. 危机响应中的位置（Ch 4 工作流）

```
60 秒标准流程（前 10 条命令）→ perf stat 全局 → perf top 定位进程
→ perf record -g 短采 → 火焰图 → 若 on-CPU 无热点 → 转 off-CPU（BPF offcputime，Ch 15）
```

对照 [ch16 案例 S0–S4 演练](../../chapter-16-case-studies/notes/section-16.9-HFT-版Unexplained-Win演练模板.md)：perf stat 是 S2（统计快照）的第一工具，record/火焰图是 S3（动态追踪）的入场券。

---

## HFT / 嵌入式关联

| 场景 | 用法 |
|---|---|
| 交易机最小观测集 | `perf stat` 是唯一能**常驻生产**的深检工具——[ch16.1.5 PMC 精读](../../chapter-16-case-studies/notes/section-16.1.5-16.1.6-PMC-与软件事件.md)的输出全靠它 |
| Pi5/嵌入式 | ARM PMU 主线支持完整，`perf stat`/`record` 全可用——eBPF 实验线（[06.7](../../../06.7-bpf-observability/)）前的第一手数据 |
| 供应链约束 | perf 是内核自带生态（linux-tools 包），过审计比拉 BCC 编译链容易 |
| 版本陷阱 | 交易机内核固化 → perf 必须用配套 linux-tools；容器里跑 perf 需特权 + 宿主内核匹配 |

---

## 衔接

- 下一节：[13.3–13.7 事件源](./section-13.3-13.7-perf-事件源.md)——四类事件源在内核里的三条实现路径
- Ch 4 [perf 在工具地图的位置](../../chapter-04-observability-tools/)
- Ch 12 [压测时 profile](../../chapter-12-benchmarking/)

---

## 代码自测

<details><summary>Q1：`perf stat -e cycles,instructions` 执行时，用户态和内核的分工是什么？</summary>

用户态 perf 命令解析参数、构造两个 `perf_event_attr`、调用两次 `perf_event_open()`（core.c:12351）拿到 fd；内核按 attr 找到对应 PMU（x86 PMC）配置硬件计数器。运行期间硬件自由计数，结束时用户态 `read(fd)` 取累计值。perf 工具本身不接触硬件。
</details>

<details><summary>Q2：为什么说"一个事件 = 一个 fd"？这个设计带来什么？</summary>

每个 perf_event_open 返回一个 fd，事件生命周期就是 fd 生命周期——`read` 取计数、`mmap` 拿采样 buffer、`ioctl(PERF_EVENT_IOC_ENABLE/DISABLE)` 控启停、`close` 销毁。fd 语义让 perf 复用了 poll/epoll 整套多路复用基础设施。
</details>

<details><summary>Q3：perf 为什么必须匹配运行内核？</summary>

perf_event_attr 结构与事件语义随内核版本演进。perf 工具头文件里编译进的 attr 布局若与运行内核不一致，轻则事件不可用，重则字段错位读出垃圾数据。`linux-tools-$(uname -r)` 对齐的就是这条 ABI 边界。
</details>

<details><summary>Q4：Pi5 上 perf 能工作的前提是什么？</summary>

内核启用 CONFIG_PERF_EVENTS 且有 ARMv8 PMU 驱动（drivers/perf/）注册成 `struct pmu` 实例。主线内核对这些支持完整——这正是嵌入式 Linux 与 HFT 观测栈交汇的便利点。
</details>

<details><summary>Q5：生产机器上 perf 三大流（计数/采样/事件）哪些可以常开？</summary>

计数流（stat/top）可常开——读计数器近零开销；采样流（record）限 PID+短窗——每样本有真实成本；事件流（trace）限短窗——每 syscall 一条事件的量。uprobe 类动态探针只在开发机。
</details>
