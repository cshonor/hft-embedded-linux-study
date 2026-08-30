# 2.12 性能监控计数器 PMC（模式 / PEBS / 云计算）

> 底本：《BPF之巅》第 2 章技术背景，2.12 节（印刷 p67–69，含 2.12.1–2.12.3）

## 是什么

PMC = Performance Monitoring Counter，别名 PIC / CPC / PMU event：**处理器上的硬件可编程计数器**。这是唯一能量化"硬件在干什么"的事件源。

## Intel 架构集合（表 2-8 节选）

| 事件 | 助记符 | 事件选择/掩码 |
|---|---|---|
| 活跃核心周期数 | CPU_CLK_UNHALTED.THREAD_P | 3CH / 00H |
| 退出指令数 | INST_RETIRED.ANY_P | C0H / 00H |
| 末级缓存引用 | LONGEST_LAT_CACHE.REFERENCE | 2EH / 4FH |
| 末级缓存未命中 | LONGEST_LAT_CACHE.MISS | 2EH / 41H |
| 分支指令 | BR_INST_RETIRED.ALL_BRANCHES | C4H / 00H |
| 分支未命中 | BR_MISP_RETIRED.ALL_BRANCHES | C5H / 00H |

由 IPC（指令/周期）、LLC 命中率、分支预测命中率可推断 CPU 效率瓶颈。

## 关键限制

PMC 有数百个，但 CPU 同时只允许**固定数量的寄存器（可能只有 6 个）**读取。要么精选，要么**循环采样**多个 PMC 集合（perf(1) 自动支持）。软件计数器无此限制。

## 2.12.1 PMC 的两种模式

| 模式 | 机制 | 开销 | BPF 关系 |
|---|---|---|---|
| **计数** | 内核按需读取频率（如每秒 1 次） | 近零 | — |
| **溢出采样** | 计数器溢出时向内核发信号（如每 10000 次 LLC miss、每 100 万周期中断一次） | 与采样率成正比 | **为 BPF 提供执行时机**（BCC/bpftrace 支持 PMC 事件跟踪） |

## 2.12.2 PEBS（精确事件采样）

问题：溢出采样存在**中断延迟（打滑 skid）**与乱序执行 → 记录的指令指针可能不是真正触发事件的指令。对周期采样无所谓（甚至故意加不规则采样频率避免 lockstep，如 99Hz），但 LLC 未命中率这类事件必须精确。
方案：Intel PEBS 用**硬件缓冲区记录 PMC 事件发生时的正确指令指针**；Linux perf_events 支持 PEBS。

## 2.12.3 云计算

许多云环境不给虚拟机 PMC 访问。技术上可行：Xen 有 vpmu 选项（作者本人提交过 Xen 中启用不同 PMC 模式的代码）；Amazon Nitro 主机开放了部分 PMC 支持。

## HFT 关联

- HFT 单核优化离不开 PMC：IPC 掉了=流水线受阻（缓存/分支 miss）；LLC miss 飙升=数据结构跨 NUMA/缓存行伪共享。`perf stat` 一分钟就能给出判断。
- 交易机部署在云/虚拟化环境时**先验证 PMC 可用性**（`perf stat ls` 看有无计数），否则整条硬件观测路径失效。
- PMC 溢出采样是 on-CPU 火焰图的数据源；HFT 中用 99Hz 这类非整频率避免锁步偏差。

## 陷阱

- 想同时监控超过寄存器数的 PMC → 需分组循环采样，别以为计数都"同时在跑"。
- 忽略 skid 直接用采样 IP 判 LLC miss 责任代码 → 结论错位几条指令；需要精确归因用 PEBS 事件。
- 虚机里 perf stat 全 0 还以为"没有缓存问题"——是 PMC 没暴露。

## 自测

<details>
<summary>1. PMC 的两种工作模式是什么？哪种对 BPF 跟踪有意义？</summary>

计数（按需读取，近零开销）与溢出采样（计数器溢出时通知内核）；溢出采样产生的事件给 BPF 程序提供执行时机。
</details>

<details>
<summary>2. 什么是 skid？PEBS 如何解决？</summary>

溢出中断有延迟且乱序执行，记录的指令指针可能偏离真正触发事件的指令；PEBS 用硬件缓冲区在事件发生当刻记录精确 IP。
</details>

<details>
<summary>3. PMC 数百个，为何一次只能读几个？</summary>

CPU 中可编程计数寄存器数量固定（约 6 个），须精选或循环采样覆盖。
</details>
