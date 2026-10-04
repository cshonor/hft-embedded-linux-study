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

perf 侧视角还有两类补充事件源：**software 软件计数器**（缺页、上下文切换等内核统计事件，bpftrace 写作 `software:page-fault`）与**定时器**（`profile:` 探针的定时采样）。两套视角合起来才是完整答案：eBPF 视角六源（表中），perf 视角再加 software/timer。

这张表有两个正交维度，值得显式拆开：

```text
            内核态                用户态              硬件
静态     tracepoints           USDT（+动态USDT）     ——
动态     kprobes/kretprobes    uprobes/uretprobes   ——
采样     ——                    ——                  PMC 溢出
```

- **静态/动态**决定稳定性与覆盖面的交换（静态稳定但少、动态全覆盖但脆）
- **内核/用户/硬件**决定你能看到哪一段（内核函数行为、应用函数行为、微架构事件）
- **事件/采样**决定开销模型（∝ 事件率 vs ∝ 采样率）

任何观测需求都能在这 3×3 网格里定位：先问"问题发生在哪个态"，再问"该态有没有静态点"，最后问"逐事件太贵要不要改采样"。

## 术语辨析：事件源 vs 事件域（易混，面试高频）

**事件源（event source）** = 底层机制大类，回答"你靠什么抓到事件"。上面表里的全部条目都是事件源，eBPF、perf、ftrace 复用同一套。

**事件域（event category / provider）** = **tracepoint 专属**的子系统分类——`域:事件名` 冒号前面那部分。回答"事件属于哪个子系统"。

| 术语 | 归属 | 例 | 回答的问题 |
|---|---|---|---|
| 事件源 | 所有插桩机制 | tracepoint / kprobe / uprobe / PMC | 靠什么抓到事件？ |
| 事件域 | **仅 tracepoint** | sched / syscalls / net / block | 事件属于哪个子系统？ |
| 事件名 | 域下面具体一点 | sched_switch | 具体是哪一个点？ |

**为什么只有 tracepoint 有域**：域是内核开发者在埋静态点时显式起的子系统名（源码中 `TRACE_EVENT` 宏配套的 `TRACE_SYSTEM`），属于静态命名空间的一部分；kprobe/uprobe 是运行时对任意函数符号动态挂探针，没有人为分类，自然没有域。bpftrace 语法直接体现了这一点：

```text
tracepoint:sched:sched_switch   三段式：源类型 : 域 : 事件名
kprobe:vfs_read                 两段式：源类型 : 函数符号（无域）
uprobe:/usr/bin/app:func        路径占的是域的位置，但那是文件路径，不是分类
```

事件域的物理形态就是 tracefs 目录树：

```text
/sys/kernel/tracing/events/
 ├─ sched/                  ← 事件域 sched
 │   ├─ sched_switch/
 │   │   └─ format          ← 自描述字段（见 1.7）
 │   └─ sched_wakeup/
 ├─ syscalls/               ← 事件域 syscalls
 └─ net/                    ← 事件域 net
```

> 陷阱注：ftrace 的 kprobe_events 动态事件会出现在 `events/kprobes/` 目录下，看起来像"kprobe 也有域"——那只是 ftrace 把用户创建的动态事件归档的伪目录，不是 kprobe 的固有概念。bpftrace/perf 语境下"kprobe 无域"依然成立。

**易错点两条**：

- ❌ "kprobe 也有事件域" → 域是静态命名空间；动态探针直接写函数符号
- ❌ 把 strace 当事件源 → strace 底层是 **ptrace**，完全独立的调试机制，不在 eBPF 事件源体系内。走 `syscalls:` 域 tracepoint 的是 **perf trace**——同样看系统调用，开销比 strace 低一个量级以上

## 事件源与内核动作：多对多关系（对"归属"的纠正）

常见误区：❌"一个事件**归属于**某一个事件源"。

✅ 真实模型：内核里的动作（进程切换、函数调用）是**客观存在**，事件源是我们搭上去的监听探头。**动作与探头是多对多**，两个正交维度：

| 维度 | 问题 | 内核机制 | 例 |
|---|---|---|---|
| 同一位置挂**多型号**探头 | 不同事件源能否同时捕获同一动作？ | 各源机制独立、互不排斥 | tracepoint:sched:sched_switch 与 kprobe:schedule 同时挂调度路径，切一次两个都响 |
| 同一型号挂**多个监听者** | 多个程序能否挂同一探针？ | tracepoint 原生回调链；kprobe 聚合探针；多工具各自 open perf event | bpftrace 和 BCC 工具同时挂 sched:sched_switch，各收各的数据 |

比喻：事件源 = 报警器**型号**（tracepoint/kprobe/uprobe…），代码位置 = 房间的**门**。同一扇门可以装不同型号的报警器（门开全响），同型号也可以装多台（门开一起响）。门是客观的，报警器是后装的。

三个机理层要点（多监听者为什么天然可行）：

1. **tracepoint 是原生回调链**：tracepoint 本质是回调函数数组，每注册一个 probe 就追加一个——多监听者是它的设计起点，不是补丁
2. **kprobe 同地址多 consumer**：多个使用者注册同一函数地址时，内核用**聚合探针**（aggregated kprobe）只放一个真实断点，事件到达时逐个回调——开销不随监听者数量翻倍
3. **BPF 工具层面各开各的 fd**：每个工具独立 `perf_event_open()` 同一事件，各得一个 fd 和环形缓冲，互不干扰——这是"bpftrace 和 BCC 工具并存挂同一探针"的常态

### 教学例的精确化：schedule() vs sched_switch 不完全同门

"sched_switch 发生时 kprobe:schedule 和 tracepoint:sched:sched_switch 同时触发"——大方向对，但两扇门其实有偏差：

- `kprobe:schedule` 挂在 **schedule() 函数入口**——每次调用都触发
- `sched:sched_switch` 埋在 **__schedule() 内部的 context_switch 处**——只有真正发生任务切换（prev != next）才触发
- `schedule()` 被调用但调度器仍选中当前任务时（空转）：kprobe 响、sched_switch **不响**

所以两探针触发次数满足 `kprobe:schedule ≥ sched:sched_switch`，"同时触发"只在真正切换的子集上成立。这个不对称本身就是**测量口径**问题：数"切换次数"用 sched_switch（语义准），数"调度器入口压力"用 kprobe（口径宽）——同一个动作，不同探头看到的世界并不严格相同，选型时要想清楚自己数的是什么。

### "老内核不支持多程序挂同一 tracepoint"的准确版本

- 准确的限制是：**单个 perf event fd** 上 `PERF_EVENT_IOC_SET_BPF` 只能绑一个 BPF 程序，重复设置是替换语义（`BPF_F_REPLACE`）
- 但每个 BPF 程序可以**自己 open 一个新的 perf event** 挂同一个 tracepoint → 多程序并存从来可行（每工具一个 fd 的代价）
- ⚠️ `BPF_F_BEFORE` / `BPF_F_AFTER`（uapi `bpf.h` 中 1U<<3 / 1U<<4，注释 "Generic attachment flags"）的实际消费者是 **cgroup / tcX 程序链框架**（bpf_mprog），用于链上相对位置插入——**不是** tracepoint 多程序的机制。把 cgroup 的 flag 安到 tracepoint 头上是常见张冠李戴

### 边界重申

ptrace 依然不属于这套体系：它是"暂停进程、接管控制"的调试机制，不是挂探针监听——不能和 kprobe/tracepoint 放进同一张多对多网格里。



## 选型原则（贯穿全书的纪律）

**能用稳定的就用稳定的**：tracepoint > kprobe；USDT > uprobe。动态插桩只在静态点不存在时作为兜底——因为动态点随版本漂移，工具会悄悄失效。

落地成三步检查：

1. `bpftrace -l 'tracepoint:*<关键词>*'` —— 有现成 tracepoint？用它
2. 没有 → 该内核函数存在且未内联？`kprobe:func` 兜底（挂载前估事件频率）
3. 事件率过高（>100k/s）→ 放弃逐事件，改采样或聚合模式

## HFT 关联

- HFT 观测体系设计可以直接抄这张表：网络收包路径（可 tracepoint + kprobe 兜底）、内存分配（uprobes 但注意开销，见 2.8.4）、锁与调度（sched tracepoint）、硬件级 IPC/缓存命中（PMC）。
- 生产 7×24 挂载的工具尽量选静态稳定源，避免内核小版本升级后探针静默丢失。
- 3×3 网格也是"观测盲区自检表"：交易系统的常见盲区是**用户态静态位**（USDT 没埋）和**硬件位**（PMC 没验证可用性）——内核位反而覆盖最全（工具最多）。

## 自测

<details>
<summary>1. 按稳定性给"tracepoint、kprobe、USDT、uprobe"排序，并说明原因。</summary>

tracepoint ≈ USDT（静态、有 API 承诺）> kprobe ≈ uprobe（动态、函数名/偏移随版本变）。内核函数重命名后 kprobe 工具会失效。
</details>

<details>
<summary>2. 为什么说"先静态后动态"是插桩选型纪律？</summary>

静态点开销低（禁用时近零成本）且跨版本稳定；动态点数量多、覆盖广但脆弱，只应作为静态点缺失时的替代。
</details>

<details>
<summary>3. 事件源全景表的两个正交维度是什么？用它们定位"交易进程等锁"该用哪类事件源。</summary>

维度一：内核/用户/硬件态；维度二：静态/动态/采样。"等锁"发生在用户态（futex 系统调用是边界）——优先 sched tracepoint（内核侧看调度切换），用户态内部看 USDT/uprobe（futex 调用点），阻塞时长聚合用 offcputime 模式。
</details>

<details>
<summary>4. 事件率 500k/s 的函数想看耗时分布，逐事件挂探针为什么不行？正确的替代是什么？</summary>

逐事件开销 ∝ 事件率：µs 级单次成本 × 500k/s = 数十秒 CPU 秒/分钟，观测税不可接受。替代：保持逐事件但只做内核态聚合（直方图 map，输出与事件率解耦）；或放弃该函数改采样（PMC/定时采样画火焰图）。
</details>

<details>
<summary>5. "kprobe:schedule 里的 schedule 是事件域吗？perf trace 和 strace 底层机制有何区别？</summary>

不是。schedule 是被插桩的内核函数符号，kprobe 是动态事件源，没有事件域——域（category）是 tracepoint 专属概念，来自源码 `TRACE_SYSTEM` 的静态命名空间（如 `tracepoint:sched:sched_switch` 的 sched）。perf trace 底层走 `syscalls:` 域的 tracepoint 事件源；strace 底层是 ptrace，独立调试机制，不在 eBPF 事件源体系内，开销高一个量级以上。
</details>

<details>
<summary>6. kprobe:schedule 与 tracepoint:sched:sched_switch 同时挂载，两探针的触发次数一定相等吗？为什么？</summary>

不一定。kprobe 挂 schedule() 入口，每次调用都触发；sched_switch 埋在 __schedule() 内部的 context_switch 处，只有真正发生任务切换（prev != next）才触发。schedule() 空转（调度器仍选当前任务）时 kprobe 响而 sched_switch 不响，所以 kprobe:schedule 次数 ≥ sched:sched_switch 次数。这揭示了"同一动作、不同探头"的口径差：数切换次数用 tracepoint（语义准），数调度器入口压力用 kprobe（口径宽）。
</details>

<details>
<summary>7. 两个 BCC 工具想同时挂 sched:sched_switch，需要什么前提？BPF_F_BEFORE/AFTER 能用在这里吗？</summary>

不需要特殊前提——每个工具各自 perf_event_open() 同一 tracepoint，各得一个 fd 和环形缓冲，天然并存（tracepoint 本身就是回调链，多监听者是设计起点）。BPF_F_BEFORE/AFTER 用不上：它们是 cgroup/tcX 程序链框架（bpf_mprog）的 attach flag，用于链上相对位置插入，与 tracepoint 挂载无关。真正的历史限制只是"单个 perf event fd 上 PERF_EVENT_IOC_SET_BPF 只绑一个程序、重复设置是替换"，绕过方式就是再 open 一个 event。
</details>
