# 2.14 小结（第 2 章技术组件全景 + 全章坑点 + 自测）

> 底本：《BPF之巅》第 2 章技术背景，2.14 节（印刷 p70）

## 原书小结

BPF 性能工具用到的技术：扩展版 BPF（虚拟机/验证器/map）、内核态动态插桩（kprobes）、用户态动态插桩（uprobes）、内核态静态跟踪（跟踪点）、用户态静态跟踪（USDT/动态 USDT）、perf_events。调用栈回溯靠帧指针和 ORC，可视化靠火焰图。

## 全章技术地图（一图流）

```
事件源（谁触发）                    BPF 程序（做什么）         输出（怎么呈现）
─────────────────────────          ─────────────────          ─────────────────
kprobes/kretprobes   [内核/动态]    验证器校验的字节码          map（内核聚合）
uprobes/uretprobes   [用户/动态]    + helper（98 个）          perf_events 环形缓冲
tracepoints          [内核/静态]    + 512B 栈/100 万指令       用户态前端（BCC/bpftrace/perf）
USDT/动态USDT        [用户/静态]    栈回溯：FP / ORC           火焰图/直方图/表格
PMC                  [硬件]         （LBR/DWARF 补充）         bpftool 检视
perf_events          [汇聚层]
```

读这张地图的正确姿势：横向三列分别是"**输入-处理-输出**"，任何 BPF 工具 = 三列各取一格的组合。第 2 部分的几十个工具全部落在这张组合表里，理解了地图就再没有"新工具"——只有新组合。

## 更大的地图：Linux 追踪全家桶（机制 vs 工具分层）

上面只画了 BPF 内部。把视野拉大到整个 Linux 追踪生态，必须分清**底层机制（地基）**与**用户态工具（成品）**两层：

```text
底层内核机制（地基）              用户态工具（成品）
─────────────────────           ─────────────────────
ptrace()      系统调用     →     strace / gdb
ftrace        跟踪框架     →     （裸用 tracefs，语法繁琐）
perf_events   事件汇聚层   →     perf（record/trace/top/stat）
eBPF          内核虚拟机   →     bpftrace / BCC / libbpf / perf(1)
```

| 机制 | 代表工具 | 能看什么 | 开销 | 一句话定位 |
|---|---|---|---|---|
| ptrace | strace、gdb | 系统调用进出、信号、断点调试 | **极高**（每次 syscall 双方停走） | 每事件都暂停进程切换上下文，线上禁用 |
| ftrace | 裸操作 tracefs | 内核函数、tracepoint、自带 function tracer | 低 | 内核自带框架，接口繁琐一般不裸用 |
| perf_events | perf | 采样、tracepoint、PMC | 低 | 统计强、自定义逻辑弱 |
| eBPF | bpftrace/BCC/libbpf | 全部探针 + **内核内自定义计算** | 很低 | 事件触发后在内核里跑你的代码 |

**关系澄清（易错，比表格本身更重要）**：

1. **ptrace 完全独立**：调试机制，不在 eBPF 事件源体系内（见 [2.6](section-6-事件源.md)）。gdb 依然必须依赖 ptrace——**eBPF 做不了断点调试**，两者不是替代关系
2. **eBPF 站在探针基础设施之上，而非站在 ftrace 之上**：kprobe/tracepoint 是独立内核基础设施，ftrace 和 eBPF 是**并行的两个消费者**；eBPF 挂 kprobe/tracepoint 走 perf_event_open（见 [2.13](section-13-perf_events.md)），不经过 ftrace
3. **ftrace 的 function tracer 是第三种插桩机制**：编译期插桩（内核编 `-mfentry`/mcount，函数入口留钩子），与 kprobe 的运行期断点（int3）不同源——ftrace 能跟踪"任意内核函数"靠的是这个，不是 kprobe
4. **perf trace ≠ strace**：同样看系统调用，perf trace 走 `syscalls:` 域 tracepoint，开销低一个量级以上；strace 每次调用都要 ptrace 停走双方
5. **分层记忆法**：strace（基于 ptrace）/ perf（基于 perf_events）/ bpftrace、BCC、libbpf（基于 eBPF）——工具选型时先想地基：调试用 ptrace 系、统计用 perf 系、内核内实时计算用 eBPF 系

> 常见误区：**"BPF 把 ptrace、ftrace 全替代了"**——错。eBPF 与 ftrace 共享探针地基但互不隶属；ptrace 是另一套调试机制且不可替代（gdb）。eBPF 的增量在"事件触发后能在内核里执行自定义字节码"这一件事上，仅此而已——但这一件事改变了观测的开销模型。


## 选型决策树

1. 有 tracepoint / USDT 吗？→ 有，用静态（稳定+零禁用开销）
2. 没有 → kprobe/uprobe 动态（先确认事件频率不高）
3. 事件每秒百万级（malloc/free 类）→ 放弃逐事件跟踪，找低频替代事件或 PMC 采样
4. 7×24 常挂 → BPF_RAW_TRACEPOINT（4.17+）
5. 跨内核版本分发工具 → CO-RE + libbpf

决策树的前三条是**事件源选型**，后两条是**程序形态与分发**——分开展率：源选错（高频挂动态）是性能事故，形态选错（无常驻优化）是开销事故，分发选错（BCC 而非 CO-RE）是运维事故。三类事故的爆炸半径完全不同。

## 全章坑点表（HFT 视角）

| # | 坑 | 后果 | 对策 |
|---|---|---|---|
| 1 | 无帧指针的二进制抓栈 | 全是 [unknown] | 关键服务 -fno-omit-frame-pointer 编译 |
| 2 | uprobe 挂 malloc/free | 目标慢 10 倍+ | 换低频事件/USDT |
| 3 | 非 per-CPU map 高并发计数 | 丢失更新、数据失真 | per-CPU map / 原子操作 |
| 4 | 火焰图 X 轴当时间读 | 误判执行顺序 | 记住 X 是字母序 |
| 5 | 5.3 前内核写循环 | 验证器拒绝 | 展开或尾调用 |
| 6 | 虚机里 PMC 全 0 | 误以为无硬件瓶颈 | 部署前验证 perf stat |
| 7 | kprobe 挂被内联函数 | attach 失败或静默丢失 | 换相邻函数/tracepoint |
| 8 | USDT 发布版没开编译开关 | 二进制无探针 | CI 断言 readelf -n |

坑点分类记忆法：

- **观测失效类**（1/7/8）：探针挂上了但拿不到数据——隐蔽，靠 CI 断言和 attach 计数防御
- **观测致害类**（2/3）：观测本身伤害被观测系统——靠"先估事件率"纪律防御
- **数据误导类**（4/6）：拿到了数据但是错的——最危险，靠读图训练和部署前验证防御

## HFT 视角的第 2 章总结

把全章技术栈压成交易机观测的三个设计决定：

1. **事件源矩阵先填表**：按 2.6 的 3×3 网格盘点——sched tracepoint（调度）、网络 tracepoint+kprobe（收包路径）、自家 USDT（策略路径）、PMC（微架构）——缺格即盲区
2. **输出通道与事件率解耦**：常驻工具一律 map 聚合（直方图/计数），逐事件输出只留应急
3. **观测事故与交易事故同级对待**：观测失效 = 延迟尖刺时是瞎子；观测致害 = 自己制造尖刺——runbook 里两者都要有预案

## 自测

<details>
<summary>1. 用一句话概括第 2 章的主线。</summary>

一切 BPF 性能工具 = （静态或动态、内核或用户或硬件的）事件源 × 内核中运行的 eBPF 程序 × map/环形缓冲输出 × 前端工具呈现；选型纪律是"优先稳定静态、避开高频动态"。
</details>

<details>
<summary>2. 你的交易服务延迟尖刺，每秒百万次调用，想看"哪个函数最耗内核时间"，第 2 章技术怎么组合？</summary>

不逐事件跟踪：PMC 溢出采样（99Hz）触发 BPF 抓帧指针栈 → 内核聚合直方图 → off-CPU/on-CPU 火焰图定位宽塔。
</details>

<details>
<summary>3. 全章八个坑点按"爆炸半径"分三类，各举一例并说明防御手段。</summary>

观测失效类（如 kprobe 挂内联函数，静默无数据）——CI 断言 + attach 计数核对；观测致害类（如 uprobe 挂 malloc，目标慢 10 倍）——挂载前估事件率；数据误导类（如虚机 PMC 全 0 误判无瓶颈）——部署前验证 + 交叉核对已知真值。
</details>

<details>
<summary>4. 为什么"事件源选型、程序形态、分发方式"三类选型错误的事故性质不同？</summary>

事件源选错（高频挂动态）是性能事故——直接伤害被观测系统的吞吐/延迟；形态选错（逐事件常驻）是开销事故——观测资源随事件率失控；分发选错（BCC 跨版本）是运维事故——内核升级后工具批量失效。防御分别靠频率评估纪律、输出通道设计、CO-RE/BTF 检查。
</details>

<details>
<summary>5. "eBPF 复用了 ftrace 的探针，所以 eBPF 建在 ftrace 之上"？哪里错了？gdb 为什么不能用 eBPF 重写？</summary>

错在层级：kprobe/tracepoint 是独立内核基础设施，ftrace 与 eBPF 是并行的两个消费者——eBPF 挂 kprobe 走 perf_event_open（2.13 的汇聚层），不经过 ftrace 接口；ftrace 自己的 function tracer 还是独立的第三种插桩机制（编译期 -mfentry，区别于 kprobe 运行期 int3）。gdb 不能用 eBPF 重写：断点调试需要"命中即停、检查/修改进程状态、单步恢复"的完全控制语义，这是 ptrace 的能力模型；eBPF 程序由验证器保证不可暂停/阻塞被观测进程，设计目标是最小观测者效应——两套机制的目标互斥。
</details>

## 交叉引用

- 前一章：[chapter-01-introduction](../../chapter-01-introduction/README.md)
- 下一章：[chapter-03-performance-analysis](../../chapter-03-performance-analysis/README.md)（60 秒分析与 USE 方法论将把本章技术落成清单）
- 全书目录：[BOOK-TOC.md](../../BOOK-TOC.md)
