# 2.6 事件源

> 底本：《BPF之巅》第 2 章技术背景，2.6 节（印刷 p46）

## 本节内容

书中用一张"事件源全景图"汇总 BPF 程序可以挂载的所有事件类型，后续 2.7–2.13 逐个展开：

| 事件源 | 态 | 插桩方式 | 稳定性 | 对应小节 |
|---|---|---|---|---|
| kprobes / kretprobes | 内核 | 动态 | 不稳定（函数可改名） | 2.7 |
| uprobes / uretprobes | 用户态 | 动态 | 不稳定（依赖符号） | 2.8 |
| tracepoints | 内核 | 静态 | 稳定（API 级承诺） | 2.9 |
| USDT | 用户态 | 静态 | 稳定（应用自带） | 2.10 / 2.11 |
| PMC（硬件计数器） | 硬件 | 采样/计数 | 稳定 | 2.12 |
| perf_events | 内核框架 | 汇聚层 | 稳定 | 2.13 |

## 选型原则（贯穿全书的纪律）

**能用稳定的就用稳定的**：tracepoint > kprobe；USDT > uprobe。动态插桩只在静态点不存在时作为兜底——因为动态点随版本漂移，工具会悄悄失效。

## HFT 关联

- HFT 观测体系设计可以直接抄这张表：网络收包路径（可 tracepoint + kprobe 兜底）、内存分配（uprobes 但注意开销，见 2.8.4）、锁与调度（sched tracepoint）、硬件级 IPC/缓存命中（PMC）。
- 生产 7×24 挂载的工具尽量选静态稳定源，避免内核小版本升级后探针静默丢失。

## 自测

<details>
<summary>1. 按稳定性给"tracepoint、kprobe、USDT、uprobe"排序，并说明原因。</summary>

tracepoint ≈ USDT（静态、有 API 承诺）> kprobe ≈ uprobe（动态、函数名/偏移随版本变）。内核函数重命名后 kprobe 工具会失效。
</details>

<details>
<summary>2. 为什么说"先静态后动态"是插桩选型纪律？</summary>

静态点开销低（禁用时近零成本）且跨版本稳定；动态点数量多、覆盖广但脆弱，只应作为静态点缺失时的替代。
</details>
