# Ch 6 中央处理器 · CPUs

> **Systems Performance 2nd** · Brendan Gregg · **精读**

> 本章定位：**CPU 驱动一切软件** — 性能分析往往从 CPU 开刀。Ch 2 的 USE/饱和度在这里落地；Ch 5 的应用热点最终要落到「哪颗核、多少 IPC、run queue 多长」。本章从硬件模型、调度器、PMC 周期分析到 perf/BPF 工具与火焰图，构成 **CPU 资源层的完整地图**。

---

## 大白话 · 本章就五件事

> **CPU 快不快，不只看频率，还要看 IPC、队列和 cache。**

**① 先搞清硬件层级：Processor → Core → 硬件线程 → Cache → Run Queue。**

- 一颗 CPU 多核心；**超线程（SMT）** 是共享执行资源的两个逻辑核 — HFT 热路径常 **禁用或独占物理核**。
- **L1/L2/L3** 决定访存延迟；**Run Queue** 决定线程等 CPU 等多久。

**② 两个比「GHz」更重要的数：IPC 和 Stall。**

- **IPC**（每周期指令数）高 = 算力用满；**低 IPC** 常意味着在等内存（**stall cycles**）。
- **使用率** 看忙不忙；**饱和度** 看 run queue / 调度延迟 — 利用率不高也可能在排队。

**③ 硬件机制 + 调度器 = 你看到的 top/mpstat。**

- **P-State** 调频率、**C-State** 省电休眠 — 低延迟机器用 `performance` governor，慎用深 C-State。
- Linux **CFS / RT 调度**、抢占、负载均衡 vs **CPU affinity / NUMA 本地性** — 量化要绑核、隔离 housekeeping。

**④ 分析三板斧：USE、Profile、Cycle Analysis（PMC）。**

- 每颗 CPU：**Utilization / Saturation / Errors**。
- **perf record** + **CPU 火焰图** 找热点函数。
- **perf stat** 测 IPC、cache miss — 判断是算力问题还是内存问题。

**⑤ 调优：先删活，再绑核、调频、cgroups。**

- `taskset` / **isolcpus** / cpusets；**PSI** 比 load average 更准；**FlameScope** 抓微秒级抖动。

下面按原书 6.1–6.7、6.9 展开（6.8 为案例/延伸，与工具章重叠处见 Ch 13/15）。

---

## 6.1–6.3 CPU 模型与核心概念

### 硬件层级

```
Socket (Processor)
  └── Core × N
        └── Hardware Thread × 1~2 (SMT / 超线程)
              └── L1I / L1D → L2 → L3 (共享)
                    └── 访问 DRAM（最慢）
```

| 概念 | 含义 | HFT |
|------|------|-----|
| **Processor / Socket** | 物理 CPU 封装 | 双路服务器 = 2 sockets |
| **Core** | 独立执行单元 | **1 热线程 : 1 物理核** 常见 |
| **Hardware thread** | 逻辑 CPU（SMT） | 与同核另一线程争资源 — 热路径避免共享 |
| **Run Queue** | 就绪等 CPU 的线程队列 | 长度 > 0 持续 = **调度饱和度** |

→ [04-Hennessy](../04-Computer-Architecture-6th/) 流水线与 cache · [01-CSAPP Ch6](../01-CSAPP-3rd/chapter-06-存储器层次结构.md)

### 时钟、流水线、IPC / CPI

| 指标 | 定义 | 解读 |
|------|------|------|
| **Clock Rate** | 时钟频率（GHz） | 同代 CPU 频率差有限，别唯频率论 |
| **Pipeline** | 指令多级流水 | 分支预测失败、依赖链会冒泡 |
| **IPC** | Instructions Per Cycle | 高 → 执行单元充实 |
| **CPI** | Cycles Per Instruction = 1/IPC | 高 → 常在 stall |
| **Stall cycles** | 流水线停顿 | 多因 **cache miss、TLB miss、等内存** |

**经验：**

```bash
perf stat -e cycles,instructions,cache-misses,cache-references -- sleep 1
# 看 IPC = instructions / cycles
```

- IPC 接近理论峰值 → CPU 算力瓶颈或指令本身重
- IPC 明显偏低 + cache-misses 高 → **先查内存布局 / 数据结构**（Ch 7），而非盲目 `-O3`

### 关键指标：利用率、饱和度、User/Kernel

| 指标 | 看什么 | 工具 |
|------|--------|------|
| **Utilization** | 非 idle 时间占比 | `mpstat`、`/proc/stat` |
| **Saturation** | run queue 长度、调度延迟 | `vmstat r`、BPF `runqlat`/`runqlen`、PSI |
| **User time** | 用户态算力 | 策略、解码 |
| **Kernel time** | 内核态 | syscall、协议栈、驱动 |
| **Steal time** | 虚拟化被宿主机偷走 | 云环境 — HFT 共置尽量为 0 |
| **Priority inversion** | 高优先级被低优先级间接阻塞 | RT 线程 + 锁 — 用优先级继承或隔离 |

**HFT 警示：**

- **单核 100% user** 不一定是好事 — 可能是 **spin 忙等**；要结合 run queue 与 off-CPU。
- **kernel % 高** 在行情机常见 — 查 softirq、网络栈、是否该上 DPDK。

→ Ch 2 [USE 方法](./chapter-02-方法论.md#24-use-方法) · [附录 A CPU 项](./appendix-A-USE方法Linux.md)

---

## 6.4 硬件与软件架构

### P-States 与 C-States

| 类型 | 作用 | 性能影响 |
|------|------|----------|
| **P-State** | 动态调频（DVFS） | `powersave` 降频增延迟；**`performance`** 锁高频 |
| **C-State** | CPU 空闲省电 | C6 等深睡眠 **唤醒延迟** — 低延迟裸机常限制 C-State |

**HFT 裸机 checklist：**

```
cpufreq governor = performance
禁用或限制 deep C-states（BIOS + intel_idle 参数，视硬件文档）
turbo 按需：要稳定延迟 vs 要峰值算力 — 与团队策略一致
```

→ [11-HFT ch05](../11-HFT-Low-Latency-Practice/chapter-05-操作系统内核极致调优.md)

### Cache、MMU、TLB、互连

| 组件 | 性能点 |
|------|--------|
| **L1/L2/L3** | 容量与延迟差数量级；**false sharing** 打穿一致性 |
| **MMU** | 虚拟地址翻译 |
| **TLB** | 页表缓存；miss 贵 — **大页（Huge pages）** 减 TLB 压力 |
| **QPI / UPI** | 多 socket 间互连 — **跨 socket 访存慢**，绑 NUMA 节点 |

→ [06-Gorman](../06-Linux-Virtual-Memory-Manager/) 页表 · [10-DPDK EAL](../10-DPDK-Low-Latency-Network/01-Intro-Book/notes/chapter-01-DPDK架构与EAL.md) 大页

### 性能监控计数器（PMCs）

**PMCs** = CPU 硬件寄存器，精确计数周期、指令、cache 事件、分支等 — **周期分析**的基础。

| 事件类 | 例子 | 用途 |
|--------|------|------|
| 基础 | `cycles`, `instructions` | IPC |
| Cache | `L1-dcache-load-misses`, `LLC-load-misses` | 局部性 |
| 分支 | `branch-misses` | 预测失败 |
| 停滞 | `stalled-cycles-frontend/backend` | 前端/后端瓶颈 |

```bash
perf stat -e cycles,instructions,cache-references,cache-misses,branch-misses ./strategy
```

→ 深入 [Ch 13 perf](./chapter-13-perf性能分析.md) · [Ch 4 PMC 数据源](./chapter-04-观测工具.md)

### Linux CPU 调度器

| 机制 | 说明 |
|------|------|
| **Time sharing** | 多线程分 CPU 时间片 |
| **Preemption** | 高优先级 / 时间片到 → 抢占当前线程 |
| **Load balancing** | 跨核迁移线程 — **破坏 cache 亲和性** |
| **CFS** | 完全公平调度 — 默认 SCHED_OTHER |
| **RT** | `SCHED_FIFO` / `SCHED_RR` — 实时类，**慎用**需 cap 防饿死 |
| **Affinity** | `sched_setaffinity` / `taskset` — 线程绑核 |
| **NUMA balancing** | 内核尝试把内存迁到线程所在节点 — 热路径有时 **关闭** 更可预测 |

**HFT 典型布局：**

```
Core 0–1   housekeeping（OS、日志、监控）
Core 2–7   行情 decode + order book（isolcpus 隔离）
Core 8–15  发单 / 风控（独立 NUMA 节点若双路）
IRQ / RPS  与数据面同 NUMA，避免 cross-socket
```

→ [05-LKD](../05-Linux-Kernel-Development/) 调度器 · Ch 3 [上下文切换](./chapter-03-操作系统.md)

---

## 6.5 性能分析方法论

### USE 方法（CPU）

对 **每个 CPU**（或每组 dedicated cores）：

| 字母 | CPU 上问什么 | 怎么量 |
|------|--------------|--------|
| **U** Utilization | 非 idle % | `mpstat -P ALL 1` |
| **S** Saturation | run queue、调度延迟 | `vmstat 1` 的 `r`；`runqlat`；**PSI cpu** |
| **E** Errors | 硬件错 | `mcelog`、EDAC、perf 不可代 |

→ [附录 A](./appendix-A-USE方法Linux.md)

### 剖析（Profiling）

**定时采样**：固定频率中断 → 采当前 PC + 栈 → 统计哪条调用栈出现最多。

| 范围 | 工具 | 输出 |
|------|------|------|
| 全系统 / 单进程 | `perf record -g` | perf.data → 火焰图 |
| BPF | `profile`（BCC/bpftrace） | 低开销、可过滤内核/用户 |

**原则：** 采样频率与时长足够；**热路径 + 符号 + 帧指针**（Ch 5 Gotchas）。

### 周期分析（Cycle Analysis）

从 **IPC** 出发，用 PMC 分解 cycles 去向：

```
高 cycles + 低 IPC
  ├── cache miss 高 → 数据结构 / 对齐 / NUMA（Ch 7）
  ├── branch miss 高 → 分支预测、不可预测 if
  ├── frontend stall → I-cache、解码
  └── backend stall → 执行端口、依赖链
```

**HFT：** 优化 order book 前后各跑一次 `perf stat`，对比 IPC 与 `LLC-load-misses` — 比凭感觉改结构靠谱。

---

## 6.6–6.7 观测工具与可视化

### 传统系统工具

| 工具 | 看什么 | HFT 用法 |
|------|--------|----------|
| **`uptime`** | **Load average**（1/5/15 min） | 粗筛；**不能替代** per-CPU 分析 |
| **PSI** | `/proc/pressure/cpu` **压力停滞** | Linux 4.20+；比 load 更反映「等 CPU」 |
| **`vmstat 1`** | `r` 运行队列、`b` 阻塞、`cs` 上下文切换 | 尖刺时先看 `r` 与 `cs` |
| **`mpstat -P ALL 1`** | **每 CPU** user/sys/idle/irq | 找热点核、是否单核打满 |
| **`pidstat -u -w 1`** | 按进程/线程 CPU + **cswch** | 定位哪个 TID 切换多 |
| **`top` / `htop`** | 实时排序 | 危机第一轮 |

**Load Average 大白话：**

- 可运行 + 不可中断睡眠（如 D 态 I/O）线程数的指数平均 — **不是 CPU 使用率**。
- 8 核机器 load 8 ≠ 100% — 要看 **mpstat** 每核分布。

**PSI 示例：**

```bash
cat /proc/pressure/cpu
# some avg10=0.00 avg60=...  →  部分线程因 CPU 不足而stall 的时间占比
```

### perf 与 BPF 工具集

| 工具 | 作用 |
|------|------|
| **`perf stat`** | IPC、PMC、整体计数 |
| **`perf record` / `report`** | CPU 剖析、调度 trace |
| **`profile`（BCC）** | BPF 栈采样 |
| **`runqlat`** | run queue **延迟分布** — 调度饱和度金标准 |
| **`runqlen`** | run queue **长度** 随时间 |
| **`softirqs` / `hardirqs`** | 中断 CPU 消耗 — 网卡收包路径 |

```bash
# 调度延迟（生产限时长）
sudo runqlat-bpfcc 10

# 每 CPU 使用率
mpstat -P ALL 1 5
```

→ [Ch 15 BPF](./chapter-15-BPF技术.md) · [附录 C](./appendix-C-bpftrace单行命令.md) · [03-BPF](../03-BPF-Performance-Tools/)

### 可视化：火焰图与 FlameScope

**CPU 火焰图（Flame Graph）：**

- X 轴：**样本占比**（非时间顺序）；Y 轴：栈深度。
- **最宽的塔** = 最热函数 — 自顶向下读「谁调用谁」。

**FlameScope：**

- **亚秒级偏移热力图** + 可选火焰图 — 在大样本里找 **抖动、方差、周期性尖刺**。
- HFT：把 **P99 延迟尖刺** 的时间窗口对齐到 FlameScope，看是 GC、调度还是 IRQ。

→ Ch 2 [FlameScope / 热力图](./chapter-02-方法论.md#210-统计与可视化) · Ch 1 [火焰图概念](./chapter-01-简介.md)

---

## 6.9 CPU 调优

### 优先级（Gregg 顺序）

1. **消除不必要的工作** — 最高 ROI（Ch 5）
2. 编译器优化（`-O2`/`-O3`、PGO — 需 benchmark）
3. 调度优先级：`nice`、`chrt`（RT 谨慎）
4. 频率：**governor = performance**
5. **CPU 绑定**：`taskset`、`isolcpus`、cpusets
6. **资源控制**：cgroups v2 CPU quota — 云/容器；**HFT 裸机常不用 quota，用隔离**

### 调优手段对照

| 手段 | 命令 / 配置 | HFT 场景 |
|------|-------------|----------|
| **nice** | 降低/提高 CFS 权重 | 监控进程调低 |
| **chrt -f** | SCHED_FIFO 实时 | 仅关键线程 + 文档化 |
| **cpufreq** | `performance` governor | 裸机默认 |
| **taskset** | 绑核 | 进程启动时绑定 |
| **isolcpus** | 内核参数，核不参与通用调度 | 数据面专用核 |
| **cpusets** | cgroup cpuset | 容器化部署 |
| **cgroups CPU** | `cpu.max` quota | 多租户；低延迟共置慎用 |
| **irqbalance 关** | 手动绑 IRQ 到 housekeeping 核 | 网卡 interrupt affinity |
| **RPS/XPS** | 软中断分散 | 与 DPDK 轮询模式互斥 |

**与 Ch 5 衔接：** 应用层伪共享、锁优化 → 这里用 **mpstat + perf** 验证是否真降了 CPU 与 run queue。

---

## 本章 Checklist

- [ ] 能画 **Socket → Core → Thread → Cache → Run Queue** 层级
- [ ] 会用 **`perf stat`** 看 IPC，并知道低 IPC 常指向内存/stall
- [ ] 对每 CPU 做过 **USE**：mpstat + run queue/PSI
- [ ] 跑过 **`perf record -g`** 并读过 CPU 火焰图最宽栈
- [ ] 知道 **load average ≠ CPU 利用率**，会看 **mpstat -P ALL**
- [ ] 裸机确认 **governor / isolcpus / IRQ affinity** 配置文档化

---

## HFT 精读捷径（Ch 6 在路线中的位置）

```
Ch 2  USE / 饱和度 / 火焰图概念
Ch 3  调度、上下文切换、syscall
Ch 5  应用热点、Off-CPU
Ch 6  CPU（本章：硬件 + 调度 + PMC + 工具）
  → Ch 7 内存（IPC 低时常跳这里）
  → Ch 10 网络（kernel % / softirq 高时）
  → Ch 13 perf 深入
  → Ch 15 BPF runqlat/profile
  → 04-Hennessy cache/MESI
  → 11-HFT ch05 内核调优落地
```

**本章最小行动集：**

1. **`mpstat -P ALL 1`** 跑 60 秒，记录是否单核热点、sys% 是否异常。
2. **`perf stat -e cycles,instructions,cache-misses`** 对 strategy 进程压测一轮，记下 IPC。
3. **`sudo runqlat-bpfcc 10`** 看 dedicated 核 run queue 延迟是否接近 0。
4. **CPU 火焰图** 一张 + 对照 Ch 5 线程状态，确认热点在 User 还是 Kernel。

**Gregg 本章金句（HFT 版）：**

> CPU 通常是第一个要查的资源 — 但 **高利用率不等于高效**，要看 **IPC、run queue 和火焰图**。  
> **绑核** 是为了 cache 和 NUMA 本地性，不是为了把 load average 做好看。

---

## 相关章节

- 上一章：[chapter-05-应用程序.md](./chapter-05-应用程序.md)
- 下一章：[chapter-07-内存.md](./chapter-07-内存.md)
- 方法论 / USE：[chapter-02-方法论.md](./chapter-02-方法论.md) · [appendix-A-USE方法Linux.md](./appendix-A-USE方法Linux.md)
- 观测工具：[chapter-04-观测工具.md](./chapter-04-观测工具.md)
- perf：[chapter-13-perf性能分析.md](./chapter-13-perf性能分析.md)
- BPF：[chapter-15-BPF技术.md](./chapter-15-BPF技术.md)
- OS 调度：[chapter-03-操作系统.md](./chapter-03-操作系统.md)
- 架构：[04-Computer-Architecture-6th](../04-Computer-Architecture-6th/)
- HFT 绑核调优：[11-HFT ch05](../11-HFT-Low-Latency-Practice/chapter-05-操作系统内核极致调优.md)
- 全书目录：[OUTLINE.md](./OUTLINE.md)
