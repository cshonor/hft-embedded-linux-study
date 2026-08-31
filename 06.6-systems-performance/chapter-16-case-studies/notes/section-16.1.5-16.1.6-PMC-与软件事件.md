## 16.1.5–16.1.6 PMC 与软件事件

> **出处：** Gregg《性能之巅》Ch 16.1.5–16.1.6 · 宏观统计解释不了「为什么在 CPU 上变快」时，下钻 **微架构层**——用 PMC（性能监控计数器）量化「哪类快」，用软件事件交叉验证内核侧原因。
> **HFT 实操要点：** PMC 的杀手锏是 **区分「频率的快」和「效率的快」**——cycles 少了还是每 cycle 干的活多了？这两个答案指向完全不同的根因和完全不同的可持续性。

```
  perf stat = 一条命令同时拿到：
  ┌────────────────────────────────────────────────┐
  │ 硬件层：cycles / instructions / cache-miss /   │  「微架构发生了什么」
  │         branch-miss / IPC（衍生）               │
  │ 软件层：context-switches / cpu-migrations /    │  「内核行为发生了什么」
  │         page-faults / task-clock               │
  │ 频率层：GHz（cycles ÷ task-clock）              │  「CPU 到底跑没跑满频」
  └────────────────────────────────────────────────┘
```

---

### 一、PMC 基础：为什么它能「测效率」

| | |
|--|--|
| **是什么** | CPU 硬件里的性能计数器（PMU），每个核一组——计数 `cycles`、`instructions`、各级 cache miss、分支预测失败等**微架构事件** |
| **谁读它** | 内核 perf 子系统（`perf_event_open(2)`）；用户态工具（`perf stat`）只跟内核要数 |
| **开销** | **计数模式几乎为零**（硬件自动累加）；**采样模式**（`perf record`）才有中断开销 |
| **限制** | 每核同时可用的 generic 计数器通常 4–8 个（`perf stat` 会自动 multiplex，输出里有 `%` 占比提示）；虚拟化下需 pass-through（云上常见不可用 → Ch 11 的 PMC 挑战） |

> 关键认识：`perf stat`（计数）和 `perf record`（采样）是两个开销等级。Unexplained Win 的排查在 16.1.5 阶段应只开 **stat**——零成本拿全部汇总数，等假设收敛到「哪条路径」才上 record（16.1.7）。

---

### 二、标准命令与输出精读

```bash
perf stat -e cycles,instructions,cache-references,cache-misses,\
branch-instructions,branch-misses,context-switches,cpu-migrations,\
page-faults -p <策略PID> -- sleep 10
```

**输出样例（精读版，标注行号）：**

```
 Performance counter stats for process id '4321':

     4,197,830,552      cycles                    ← ①总周期
     8,412,995,104      instructions              ← ②总指令
       412,883,201      cache-references          ← ③LLC 访问
        18,577,740      cache-misses              ← ④LLC 未命中（4.5%）
     1,006,431,220      branch-instructions
          2,991,432      branch-misses            ← ⑤0.3%
            15,204      context-switches          ← ⑥切换次数
                 3      cpu-migrations            ← ⑦核间迁移
               914      page-faults               ← ⑧缺页
        10.002431077  seconds time elapsed
        419.58 GHz 后的 GHz 数在 ④⑤ 之间（perf 版本不同位置不同）  ← ⑨频率
```

**判读表（六信号联动）：**

| 信号 | 变化 | 可能含义 | 下一步 |
|------|------|----------|--------|
| ⑨ **GHz** | ↑ | 「快」其实是频率（governor/降频状态变化） | 回 16.1.4 查 governor/thermal——**频率假象，出局** |
| ② instructions | ↓ 且吞吐同 | 干的活变少（workload 变了） | 回 16.1.3 查负载特征 |
| IPC（=②/①） | ↑ | 每 cycle 完成更多——**stall 减少**：cache/分支/调度改善 | 看哪个 miss 同步降 |
| ④ cache-misses | ↓ | 数据布局/NUMA/邻居 LLC 争用减少 | 对照 `smaps`、邻居核利用率 |
| ⑤ branch-misses | ↓ | 热路径分支更可预测（代码或数据分布变化） | 进 16.1.7 找哪条路径 |
| ⑦ cpu-migrations | ↓ | 绑核生效/调度变稳——冷 cache 效应消失 | 对照 16.1.2 H4 假设 |

**IPC 判读四象限（核心工具）：**

| | instructions ↓ | instructions → | instructions ↑ |
|--|----------------|----------------|----------------|
| **IPC ↑** | 活少效率高（workload 或剔除慢路径） | **经典「效率型 win」**：stall 减少 | 变化最大（优化+快路径） |
| **IPC →** | 等比减少（频率/时间窗假象） | 什么都没变（假 win，查测量） | 快但周期等比增（如复制更多数据） |

> **第一步永远是先看 GHz 行。** cycles 变少最平凡的解释是频率变了——`perf stat` 默认输出里 `GHz` = cycles ÷ elapsed，先排除这个，微架构分析才有意义。（HFT 生产机上 governor 应恒为 performance，但「应然≠实然」——16.1.4 的教训。）

---

### 三、Top-down 思想（PMC 的进阶读法）

当 IPC 变化需要更精细解释时，工业界用 Intel 的 Top-Down 分析把「每 cycle 的去向」拆成四类（工具：`perf stat --topdown` 或 `toplev.py`）：

| 类别 | 含义 | 典型原因 | HFT 关联 |
|------|------|----------|----------|
| **Retiring** | 正常退休的指令占比 | 越高越好（>50% 算健康） | 优化直接收益区 |
| **Frontend Bound** | 取指/译码受阻 | i-cache miss、分支目标 miss | 大二进制/模板代码 |
| **Backend Bound** | 数据供给受阻（最常见） | d-cache/LLC miss、内存带宽、端口争用 | 随机访问模式、NUMA 跨节点 |
| **Bad Speculation** | 错误推测浪费的周期 | 分支预测失败、流水线冲刷 | 数据相关分支密集的策略代码 |

Unexplained Win 若表现为 Backend Bound 下降 → 指向数据供给（缓存/带宽/邻居）；若 Bad Speculation 下降 → 指向代码路径变化。**PMC 的价值就是把「快了」翻译成「哪类微架构事件少了」**，从而反推机制。

---

### 四、软件事件（Software Events）：内核侧交叉验证

硬件计数器说「CPU 层面快了」，软件事件回答「**内核做了什么配合**」：

| 事件 | 内核来源 | 关联的故事 | 工具 |
|------|----------|------------|------|
| `context-switches` | `schedule()` 每次被调用 +1 | 切换少 = 唤醒/抢占/阻塞减少 | `perf stat` |
| `cpu-migrations` | 任务换核执行 | 迁移少 = 亲和生效、负载均衡变稳 | `perf stat` |
| `page-faults` | `do_page_fault` 路径 | 缺页少 = 页表/THP/预取状态变化 | `perf stat` |
| `major-faults` | 缺页走到磁盘 I/O | 应恒为 0（HFT 热路径） | `perf stat` |
| sched tracepoints | `sched:sched_switch` / `sched_migrate` | 逐事件的切换/迁移明细 | perf / BPF |
| syscalls | `raw_syscalls:sys_enter` | syscall 数量/种类变化 | `perf trace` / BPF |

**组合阅读矩阵（把 PMC 与软件事件拼起来）：**

```
IPC ↑ + context-switches ↓        → 调度/绑核故事（冷 cache 效应消失）
IPC ↑ + cache-misses ↓ + 邻居核同时变闲 → LLC 争用消退故事（邻居假设）
IPC ↑ + branch-misses ↓            → 代码路径/数据分布变化故事
IPC → + 吞吐 ↑                     → 并行度/等待结构变化（不是微架构）
cycles ↓ + instructions → + GHz →  → 什么都没变，时间窗/口径假象
IPC ↑ + page-faults ↓              → 内存状态故事（THP/预热/compaction）
```

> 这就是案例方法论的核心动作：**单指标永远有多个解释，组合才有唯一指向**。16.1.2 的假设矩阵在这一步被压缩成 1–2 个「故事」。

---

### 五、HFT 生产注意事项

| 注意 | 原因 | 对策 |
|------|------|------|
| **perf stat 零开销但别滥用 multiplex** | 事件超过计数器数时内核分时复用，精度下降 | 拆两轮跑，或看输出里的 `xx% time enabled` 行 |
| **云上 PMC 可能不可用** | hypervisor 不透传 PMU | `perf stat` 报错/全 0 时改用软件事件 + trace 侧路线 |
| **采样（record）对延迟敏感进程有扰动** | NMI 采样中断 | stat 阶段先收敛假设，record 挪到复现环境 |
| **频率先行的纪律** | cycles 语义依赖频率恒定 | 生产机 governor 锁 performance + 第一步看 GHz 行 |
| **per-thread vs per-process** | `-p` 加 `-t` 是线程粒度 | 策略进程多线程时按热线程单独 stat |

---

### 六、衔接

- 上一节：[16.1.3–16.1.4](./section-16.1.3-16.1.4-统计数据与静态配置.md)——统计/配置淘汰了便宜假设，才轮到 PMC 上场。
- 下一节：[16.1.7–16.1.8](./section-16.1.7-16.1.8-动态追踪与结论.md)——PMC 说出「哪类快」，追踪说出「哪条路径快」。
- 深入工具：[Ch 13 perf stat 事件计数](../../chapter-13-perf/notes/section-13.8-perf-stat-事件计数.md) · [Ch 6 CPU 微架构](../../chapter-06-cpus/) · [Ch 7 内存缺页](../../chapter-07-memory/)。

---

<details>
<summary>代码自测（Q&A，先遮住答案想）</summary>

**Q1：cycles 下降 20%、instructions 不变、吞吐上升 20%——最可能的解释是什么？该怎么进一步确认？**

A：先怀疑**频率假象**：若 GHz 上升 20%（如 governor 从 powersave 变 performance、或之前 thermal throttling），则 cycles 按比例减少、指令数不变、墙钟时间缩短——一切「看起来变快」但微架构效率（IPC）其实没变。确认：看 `perf stat` 的 GHz 行或 `turbostat`；回 16.1.4 查 governor/温度历史。这是 Unexplained Win 最经典的「假 win」形态之一。

**Q2：为什么 `perf stat`（计数）几乎零开销而 `perf record`（采样）有可观开销？**

A：计数模式下，事件由 PMU 硬件自动累加，CPU 只是周期性执行一条 `RDPMC`/寄存器读——没有中断、没有停顿。采样模式下，内核需要按采样率（如 -F 99，每秒 99 次）向 CPU 发 NMI 中断，每次中断要保存/恢复上下文、抓栈、写 ring buffer——中断本身还污染 cache。所以纪律是：**stat 收敛假设，record 只做确认**。

**Q3：IPC 从 1.9 升到 2.4，cache-misses 降了 60%，但配置 diff 为空、代码没变——下一个最该查什么？**

A：**环境争用假设**：LLC/内存带宽被谁分享过？① 同 NUMA 节点/同 CCX 的邻居进程是否撤离或变安静（`mpstat -P ALL` 看别的核）；② 云上是否换了宿主机（steal、microcode、dmesg 启动时间侧面证据）；③ compaction/THP 是否让策略工作集的物理布局变好（`smaps` 的 AnonHugePages 前后对比）。IPC+cache-miss 的组合强烈指向「数据供给变好」，而代码没变时数据供给的变化几乎总是**环境**给的。

**Q4：软件事件 `context-switches` 里，voluntary 和 involuntary 的区分为什么对 HFT 重要？**

A：voluntary（任务主动让出：等锁/IO/睡眠）和 involuntary（被抢占：时间片耗尽或更高优先级唤醒）机制完全不同。HFT 热路径上 involuntary cs 是延迟抖动主源（时间片抢占），对策是 SCHED_FIFO/绑核；voluntary cs 多说明热路径上有本不该有的阻塞，对策是消除等待（如锁换无锁结构）。「切换变少」如果只细看是 involuntary 降了，故事就指向调度策略变化；若 voluntary 降了，指向等待结构变化。

**Q5：云上 PMC 全 0（不可用），如何完成 16.1.5 这一步的等价分析？**

A：退化为软件路线：① `perf stat` 只留软件事件（context-switches/page-faults/task-clock——软件事件不经 PMU，虚拟化下可用）；② `/proc/<pid>/stat` 的 utime/stime 序列采样换算 CPU 时间；③ trace 侧（16.1.7）直接上 perf record（基于 NMI 的采样同样可能受限，可改用 timer 事件）或 bpftrace kprobe。信息少了「微架构哪类快」，但仍能回答「内核行为变没变」。

</details>

---

← [本章导读](../README.md)
