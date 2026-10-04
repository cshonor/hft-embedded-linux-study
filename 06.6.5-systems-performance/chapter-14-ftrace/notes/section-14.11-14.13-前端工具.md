# 14.11–14.13 前端工具：trace-cmd、KernelShark、perf ftrace、perf-tools

> [章节导航](../README.md) · 上一节：[14.9 硬件延迟检测 hwlat](./section-14.9-硬件延迟检测hwlat.md) · 下一节：[Ch 15 BPF](../../chapter-15-bpf/README.md)

## 本节讲什么

裸 tracefs 的痛点：多 event 配置要写 N 个文件、buffer 读出来是一坨文本、没法归档对比。本节四个前端解决不同侧面，并给出一张**全章命令速查卡**收尾。

---

## 1. trace-cmd：命令行前端

**本质：把 tracefs 的 N 次文件写打包成一条命令**，外加二进制归档格式 `trace.dat`。

```bash
# 录制 sched 事件 5 秒（record 自己管 tracing_on 开关）
trace-cmd record -e sched sleep 5
trace-cmd report | head -40          # 解码回放

# 组合：函数图 + 双事件系统 + 指定 CPU/PID
trace-cmd record -p function_graph -l tcp_recvmsg -e sched -e net sleep 10

# 归档：产出 trace.dat（含事件 format、buffer、CPU 信息——自包含）
trace-cmd record -o case42.dat -e sched -e irq -e hwlat sleep 60

# 事后提取（机器已崩前留在 buffer 里的内容）
trace-cmd extract -o crash.dat
```

| 能力 | 说明 |
|---|---|
| `-e 系统/事件` | 批量启用事件（`-e sched` = 整个 sched 子系统） |
| `-p tracer` + `-l 函数` | graph/function tracer + `set_graph_function` 一步到位 |
| `-P pid` / `-C cpu` | 内置过滤（写的是 tracefs filter） |
| `trace.dat` | **二进制自包含归档**：事件元数据 + 原始 buffer——离线机器上 `trace-cmd report` 即可复现 |
| `trace-cmd listen` | 网络模式：远程采集集中存（多机排障） |

> **HFT 工作流价值**：故障现场只留一个 `trace.dat`（几 MB~几百 MB），回办公室慢慢 `report`/KernelShark 分析——与 [ch16 快照/可回放纪律](../../chapter-16-case-studies/notes/section-16.1.3-16.1.4-统计数据与静态配置.md) 直接对齐。

## 2. KernelShark：GUI 前端

`trace.dat` 的图形界面：

| 能力 | 用法 |
|---|---|
| **时间轴视图** | 每 CPU 一行、任务按颜色分——肉眼找"齐步走"的异常模式（全场同时静默 = 疑似 SMI，对照 [hwlat](./section-14.9-硬件延迟检测hwlat.md)） |
| 图形过滤 | 双击任务/事件即隐藏——万条事件里聚焦目标 PID |
| 双视图联动 | 时间轴 + 原始事件列表同步选中 |
| 调度分析 | graph 模式可视化任务在 CPU 间的迁移/阻塞 |

定位：**人工浏览长时间 trace**（几分钟的录制、几十万事件）——`cat trace` 翻不动的地方。

## 3. perf ftrace：统一入口

`perf` 内置的 Ftrace 壳，把 graph tracer 挂进 perf 命令族：

```bash
perf ftrace --tracer function_graph -- sleep 5
perf ftrace -G __x64_sys_epoll_wait -- sleep 3    # -G 指定 graph 根
perf ftrace --tracer function -G tcp_v4_rcv -- sleep 3
```

价值：**已经用 perf 的机器不用装新包**；命令历史/脚本统一。限制：只包了 function/graph 两个 tracer——hist、hwlat、kprobe_events 还得裸 tracefs 或 trace-cmd。（`perf trace` 是另一回事——走 tracepoint 的 strace 替代品，见 [Ch 13](../../chapter-13-perf/)。）

## 4. perf-tools（Gregg 脚本集）

https://github.com/brendangregg/perf-tools

**本质：读 tracefs 的 bash 封装**——每个工具就是"写好 filter/trigger + 格式化输出"的一段脚本。这既是它最大的优点（零依赖、可读源码学 tracefs 写法），也是天花板（无内核内聚合逻辑，全靠文本处理）。

| 工具类 | 代表 | tracefs 原理（读源码可见） |
|---|---|---|
| 系统调用 | `syscount`、`funclatency` | function tracer + filter + awk |
| 文件 | `opensnoop`、`execsnoop` | tracepoint `sys_enter_openat` + `if` filter |
| 调度 | `runqlat`（Ftrace 版）、`offcputime`（Ftrace 版） | kprobe `finish_task_switch` + 文本聚合 |
| 网络 | `tcpconnect`、`tcpretrans` | tracepoint `tcp:tcp_retransmit_skb` |

**何时仍需要它**：

| 场景 | 理由 |
|---|---|
| 老内核无 BPF（< 4.4 / 发行版未开 BTF） | BCC/bpftrace 装不了，Ftrace 是唯一动态观测面 |
| 最小环境（容器救援、嵌入式 rootfs） | bash + tracefs 就能跑 |
| **学习** | 100 行 bash 里能看清每个 tracefs 文件怎么用——本系列笔记的实操参考 |

现代 HFT 裸机：优先 **bpftrace/BCC**（[Ch 15](../../chapter-15-bpf/)、[06.7](../../../06.7-bpf-observability/)）——内核内聚合 + map 摘取 + uprobe 批量优化，观测开销低一个档位。perf-tools 作 fallback 与教材。

---

## 5. 全章速查卡

```bash
TR=/sys/kernel/tracing                     # 以下 $TR 省略
# —— 基础闸门 ——
cat  $TR/available_tracers                 # 看家底
echo function_graph > $TR/current_tracer   # 选 tracer
echo 1 > $TR/tracing_on                    # 开总闸（记得关：echo 0）
# —— 函数/图 ——
echo 'tcp_*'  > $TR/set_ftrace_filter      # function 白名单
echo tcp_recvmsg > $TR/set_graph_function  # graph 根
echo nofuncgraph-sleep-time > $TR/trace_options   # 剥离睡眠时间
# —— 事件 ——
echo 1 > $TR/events/sched/sched_switch/enable     # tracepoint
echo 'p:myrecv tcp_v4_rcv skb=%ax:s64' >> $TR/kprobe_events
echo 'pid == 4242' > $TR/events/sched/sched_switch/filter
echo 'stacktrace if next_pid == 4242' > $TR/events/sched/sched_switch/trigger
# —— 内核内直方图（调度延迟）——
echo 'hist:keys=next_pid' > $TR/events/sched/sched_switch/trigger
# —— hwlat ——
echo hwlat > $TR/current_tracer && echo 5 > $TR/tracing_threshold
# —— 读 ——
cat $TR/trace | head -50                   # 快照读
cat $TR/trace_pipe | grep --line-buffered xxx   # 流式读
# —— 前端 ——
trace-cmd record -e sched sleep 5 && trace-cmd report
perf ftrace -G tcp_recvmsg -- sleep 3
```

## 6. 选型决策表（全章收束）

| 需求 | 首选 | 备选 |
|---|---|---|
| CPU 剖析 / 火焰图 / PMC | [perf record（Ch 13）](../../chapter-13-perf/) | — |
| 内核路径**逐函数耗时** | `function_graph`（窄根） | trace-cmd 包一层 |
| 高频事件**直方图** | BPF map / hist trigger | hist（零依赖） |
| **SMI/固件停顿** | `hwlat` | turbostat 交叉 |
| 长时间 trace 人工浏览 | trace-cmd + **KernelShark** | — |
| 生产通用追踪/自定义聚合 | **bpftrace/BCC（Ch 15）** | — |
| 老内核/最小环境 | **perf-tools + 裸 tracefs** | — |
| 故障归档复盘 | `trace.dat` | buffer dump |

---

## HFT / 嵌入式关联

| 场景 | 用法 |
|---|---|
| 事故复盘 | 现场 `trace-cmd extract` 保住 buffer → `trace.dat` 归档 → KernelShark 时间轴找全场静默点 |
| 交易机白盒约束 | trace-cmd/perf-tools 是**纯文本工具**，过供应链审计比带编译器的 BCC 容易 |
| 多机排障 | `trace-cmd listen` 汇聚多台采集 |
| 学习路线 | perf-tools 源码 = tracefs 最好的可执行文档——写自定义观测前先抄它 |
| 与 Pi5 实验线 | 全部命令在 Pi 上可用（[06.7 eBPF 实操](../../../06.7-bpf-observability/) 前的热身） |

---

## 衔接

- 本章完 → 下一章：[Ch 15 BPF](../../chapter-15-bpf/README.md)——生产观测主力，kprobe/uprobe 与本章同源
- 回看：[14.1 tracefs 与动态插桩机制](./section-14.1-14.2-核心能力与-tracefs.md)
- 方法论总装：[ch16 案例研究](../../chapter-16-case-studies/notes/section-16.9-HFT-版Unexplained-Win演练模板.md)——Ftrace 家工具全部在 S0–S4 演练里出现

---

## 代码自测

<details><summary>Q1：trace-cmd 相比裸 tracefs 多给了什么？</summary>

三样：① 一条命令聚合多 event/tracer 配置（否则要写 N 个 tracefs 文件）；② `trace.dat` 二进制自包含归档（事件元数据+buffer，可离线复现）；③ `extract` 从事故机器 buffer 里捞现场。
</details>

<details><summary>Q2：KernelShark 时间轴上"所有 CPU 同时出现空档"暗示什么？下一步动作？</summary>

全场同时静默=OS 整体冻结的嫌疑——典型是 SMI。下一步跑 hwlat（`mode=per-cpu` + 低阈值）拿 `nmi_total_ts` 证据，再查 BIOS/BMC。单核空档则更像调度/中断问题，走 runqlat/offcputime。
</details>

<details><summary>Q3：perf-tools 的 runqlat 和 bpftrace 的 runqlat 输出几乎一样，本质区别在哪？</summary>

聚合位置：perf-tools 把每条事件**送用户态**做 awk 统计（文本处理，观测开销随事件率线性涨）；bpftrace 在**内核内**更新 BPF map 桶（每事件一次自增），用户态只摘一次结果。高事件率下后者开销低一个量级。
</details>

<details><summary>Q4：`perf ftrace` 和 `perf trace` 是一个东西吗？</summary>

不是。`perf ftrace` 是 function/function_graph tracer 的壳（写 current_tracer/set_graph_function）；`perf trace` 是 tracepoint 事件的 strace 式消费（打印 syscall 参数）。前者逐函数耗时，后者系统调用序列。
</details>

<details><summary>Q5：为什么 Ftrace 家工具在"无 BPF 老内核"上是唯一选择？</summary>

Ftrace 自 2.6.27 起就在主线内核里（tracepoint/kprobe_events/hist 都是纯内核特性，无用户态依赖）；BPF 观测栈需要 4.x+ 内核 + BTF/jit 等条件。老机器上 tracefs 是唯一不需要升级内核就能用的动态观测面。
</details>
