# 2.13 perf_events（BPF 工具的事件汇聚层）

> 底本：《BPF之巅》第 2 章技术背景，2.13 节（印刷 p69–70）

## 是什么

perf_events 是 perf(1) 命令所依赖的**采样与跟踪机制**，2009 年随 Linux 2.6.31 合入。

## BPF 工具与 perf_events 的关系（演进三步）

1. BCC/bpftrace 先把 perf_events **用作环形缓冲区**（内核→用户态传数据）
2. 又通过它**增加了对 PMC 的支持**
3. 现在**通过 perf_event_open() 观测所有事件**（kprobe/uprobe/tracepoint/USDT/PMC 统一挂载）

→ perf_events 实际成了 BPF 跟踪工具的事件汇聚层/统一入口。

## perf(1) 本身也是 BPF 前端

perf(1) 开发了使用 BPF 的接口，成为又一个 BPF 跟踪器。与 BCC/bpftrace 不同：**perf(1) 代码在 Linux 内核源码树中，是唯一内置的 BPF 前端**（无需额外安装）。其 BPF 功能仍在开发、使用尚不便利（附录 D 有 perf+BPF 例子）。

## HFT 关联

- 理解"一切事件经由 perf_event_open()"意味着：排障时用 `perf list` / `bpftool perf show` 就能盘点系统上所有观测点（含别人挂的），避免观测互相踩踏。
- 生产交易机用 perf(1) 内置 BPF 能力可少装一个包（BCC 全家桶很重），轻量采集场景（如 PMC 采样画火焰图）perf(1) 一把梭。

## 陷阱

- 多个工具同时经 perf_events 挂同一事件时输出会互相影响；排查"数据怪异"先查 `bpftool perf show`。
- perf(1) 在内核树中 → 版本与内核强绑定，跨机器迁移脚本时注意版本差异。

## 自测

<details>
<summary>1. BCC/bpftrace 使用 perf_events 的三个阶段是什么？</summary>

环形缓冲区 → PMC 支持 → 通过 perf_event_open() 观测所有事件（统一事件汇聚层）。
</details>

<details>
<summary>2. 为什么说 perf(1) 是唯一内置的 BPF 前端？</summary>

它的代码位于 Linux 内核源码树中，随内核发布；BCC/bpftrace 都是外部项目。
</details>
