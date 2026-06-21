# Ch 4 观测工具 · Observability Tools

> **Systems Performance 2nd** · Brendan Gregg · **精读**

> 本章定位：**观测工具地图** — 不只教「跑哪条命令」，更讲清 **数据从哪来、工具有何原理、开销多大、能否信**。Ch 2 的 USE/延迟分解需要工具落地；Ch 3 的内核概念是理解数据源的前提；本章选工具；Ch 13–15 深入 perf/Ftrace/BPF。

---

## 大白话 · 本章就五件事

> 工具会换，分类和数据源不会。

**① 危机来了再装工具 — 往往已经晚了。**

- 生产出问题时临时 `apt install perf` = 拖长故障。
- **提前**在镜像/裸机装好：`procps`、`sysstat`、`linux-tools-common`（perf）、`bcc-tools`、`bpftrace`。

**② 工具按两个维度选：看谁 + 怎么看。**

| 维度 | 选项 |
|------|------|
| **看谁** | **系统级**（整机） vs **进程级**（某个 PID） |
| **怎么看** | 计数器 → 剖析 → 追踪 → 监控（细节↑ 开销↑） |

**③ 数据从内核/硬件「接口」来 — 不是工具魔法。**

- 传统：`/proc`、`/sys`
- 硬件：**PMC**（周期、cache miss）
- 静态探针：**tracepoint**（内核）、**USDT**（用户态库/应用）
- 动态探针：**kprobe/uprobe**（强大但不稳定，最后手段）

**④ 四条工具链分工：perf / Ftrace / BCC / bpftrace。**

- **perf**：CPU 剖析、PMC、部分 trace
- **Ftrace**：内核路径、调度、irq
- **BCC / bpftrace**：eBPF 可编程全栈 — HFT 长期主战场

**⑤ 别盲信数字 — 工具也会错，观测本身有成本。**

- 手册错、内核指标 bug、**观测者效应**（tracing _every_ syscall 会把系统打慢）— 都要批判性看待。

下面按原书 4.1–4.6 展开。

---

## 4.1 工具覆盖范围与「危机工具」

### 危机工具（Crisis Tools）

**Gregg 观点：** 性能危机时再装调试工具 = **为时已晚**，还可能延长 MTTR（装包、依赖、版本不匹配）。

**应提前部署的 Linux 工具包：**

| 包 / 组件 | 提供什么 |
|-----------|----------|
| **procps** | `ps`、`top`、`vmstat`、`pidstat` 等 |
| **sysstat** | `iostat`、`mpstat`、`sar`、`sadc` |
| **linux-tools-common** | `perf`（版本需匹配内核） |
| **bcc-tools** | BCC 自带脚本（biolatency、runqlat…） |
| **bpftrace** | 单行/脚本 eBPF 追踪 |

**HFT 裸机 checklist：**

```
[ ] perf 版本 = 运行中内核
[ ] bpftrace + bcc 可加载最小 BPF 程序
[ ] sar/sadc 已配置历史归档（非热路径机器也建议有）
[ ] 危机 runbook 写清：先 60 秒清单 → 再 perf/bpftrace
```

→ Ch 1 [60 秒清单](./chapter-01-简介.md#111-案例研究与-60-秒清单)  
→ 附录 [bpftrace 单行命令](./appendix-C-bpftrace单行命令.md)

---

## 4.2 工具的分类与原理

### 维度一：系统级 vs 进程级

| 范围 | 回答的问题 | 例子 |
|------|------------|------|
| **System-wide** | 整机 CPU/内存/网络/磁盘如何 | `vmstat`、`mpstat`、`sar -n DEV` |
| **Per-process** | 哪个进程/线程在干什么 | `ps`、`top -H`、`pidstat` |

**HFT：** 先 **pidstat/perf top** 定位 hot PID/TID → 再 **进程级 perf record** 或 **uprobe**。

---

### 维度二：数据收集方式

```
开销 / 细节
  低 ◄────────────────────────────────────► 高
固定计数器    剖析        监控         追踪
```

#### 固定计数器（Fixed Counters）

- **原理：** 内核持续维护的**累加整数**（包数、I/O 次数、上下文切换次数…）
- **开销：** 极低（读数即可）
- **系统级：** `vmstat`、`mpstat`、`iostat`
- **进程级：** `ps`、`top`

**适合：** 第一反应、USE 里的 **U/S/E** 粗读、Ch 1 60 秒清单。

---

#### 剖析（Profiling）

- **原理：** **定时采样**（如 99 Hz）目标栈或 PC，统计「时间花在哪些函数」
- **开销：** 采样率固定 → **可预测、通常较低**
- **典型用途：** CPU 火焰图、热点函数

**工具：** `perf record -F 99`、`perf top`

**HFT：** 线下优化策略热点；生产可用 **低频率采样** + 短窗口，避免 1000 Hz 长期开。

---

#### 追踪（Tracing）

- **原理：** **按事件记录**，每一次 syscall、每个包、每次调度切换都可记
- **开销：** 细节最全 → **CPU/存储开销可能很高**
- **典型工具：** `tcpdump`、`perf trace`、**Ftrace**、**BPF 工具**

| 场景 | 建议 |
|------|------|
| 查「哪条 syscall 多」 | 短时间 `perf trace` 或 bpftrace 计数 |
| 查「每个包」 | tcpdump（仅排查窗口，非常态） |
| 查「内核路径」 | Ftrace / bpftrace tracepoint |

**HFT：** 追踪 **必须限时长 + 限事件类型**；tick 高峰开全量 syscall trace = 自找延迟。

---

#### 监控（Monitoring）

- **原理：** **随时间持续记录**指标，归档供回溯
- **开销：** 取决于采样间隔与指标数量；后台 `sadc` 通常很轻
- **典型工具：** `sar`、Prometheus、Grafana

**适合：** 容量规划、**「昨天 P99 为何变差」**、非危机日常基线。

→ 与 **追踪** 区别：监控 = 粗粒度时间序列；追踪 = 单次事件级明细。

---

### 分类速查表

| 方式 | 细节 | 开销 | 系统级示例 | 进程级示例 |
|------|------|------|------------|------------|
| 固定计数器 | 低 | 极低 | vmstat, mpstat | ps, top |
| 剖析 | 中 | 低–中 | perf top (system) | perf record -p PID |
| 追踪 | 高 | 中–高 | Ftrace, bpftrace | perf trace, uprobe |
| 监控 | 低–中 | 低（配置得当） | sar | pidstat 系列归档 |

---

## 4.3 核心观测数据源

> 工具是「读者」；下面才是「书」— 内核与硬件暴露的接口。

### /proc 与 /sys

| 接口 | 内容 | 谁在读 |
|------|------|--------|
| **/proc** | 进程与系统状态（内存文件系统） | `top`、`vmstat`、`ps`、大量脚本 |
| **/sys** | 内核子系统、设备、 tunable | 驱动、cpufreq、block 层统计 |

**特点：** 人类可读、无专用工具也能 `cat`；但字段随内核演进，**以当前内核文档为准**。

**HFT 常用路径（示例）：**

- `/proc/interrupts`、`/proc/softirqs` — 中断分布
- `/proc/PID/status`、`/proc/PID/sched` — 绑核、调度
- `/sys/devices/system/node/` — NUMA 拓扑

---

### 硬件计数器（PMCs）

- **Performance Monitoring Counters** — CPU 硬件寄存器
- 可测：**CPU 周期、退役指令、L1/L2/LLC cache 命中/未命中、分支误预测** 等

**工具：** `perf stat`（最常用）

**HFT 示例：**

```bash
perf stat -e cycles,instructions,cache-misses,branch-misses -p <PID>
```

**解读：** IPC 低 + cache-misses 高 → 内存/布局问题；branch-misses 高 → 热分支。→ [Ch 6 CPU](./chapter-06-中央处理器.md)

---

### 静态插桩

#### Tracepoints（内核跟踪点）

- 预先写在内核**关键路径**上的检测点（syscall 入口、block I/O、调度切换…）
- **API 稳定** — 适合可重复脚本与跨版本对比

**工具：** `perf trace`（部分）、Ftrace、`bpftrace` 挂 tracepoint

---

#### USDT（User Statically Defined Tracing）

- 用户态版 tracepoint — 嵌在 **libc、MySQL、JVM** 或**自研二进制**（需编译期插桩）
- 稳定、语义清晰（如 `mysql:query__start`）

**HFT：** 若在策略/网关代码里加 USDT，可零侵扰对齐 **tick-to-trade** 分段；否则用 **时间戳日志 + uprobe** 替代。

---

### 动态插桩

#### kprobes

- 运行时在**几乎任意内核函数/指令**插探针
- **极强大**，但 **API 不稳定**（内核版本/符号变）→ **最后手段**

**工具：** Ftrace `set_ftrace_filter`、bpftrace `kprobe:`

---

#### uprobes

- 用户态动态插桩 — 追踪**任意用户函数**（含动态库）

**工具：** `perf probe`、`bpftrace uprobe:`

**HFT：** 对闭源或不便改源码的库，短期 uprobe 量化函数耗时；生产慎用高频 uprobe。

---

### 数据源选择 · HFT 优先级

```
第一反应     /proc + 固定计数器（vmstat, mpstat, pidstat）
CPU 热点     perf 剖析 + PMC（perf stat）
内核路径     tracepoint > kprobe
用户热函数   USDT（若有）> uprobe（短期）
历史回溯     sar / 自研 metrics
深度定制     bpftrace / BCC
```

→ 深入 BPF：[Ch 15](./chapter-15-BPF技术.md) · [09-BPF-Performance-Tools](../09-BPF-Performance-Tools/)

---

## 4.4 sar 工具

**sar（System Activity Reporter）** — 虽有了 BPF，仍是**必备**传统利器。

| 能力 | 说明 |
|------|------|
| **实时** | `sar -u 1`、`sar -n DEV 1`、`sar -B 1` 等 |
| **历史** | 后台 **sadc** 定期采样，`sar -f` 读归档 |
| **覆盖** | CPU、内存、swap、I/O、网络、队列、进程… |

**为何仍重要：**

- 低开销、久经考验
- **「上周同一时段对比」** — BPF 常缺长期基线 unless 自建
- 与 [USE 方法](./appendix-A-USE方法Linux.md) 清单字段高度重合

**常用示例：**

```bash
sar -u 1 5          # CPU
sar -n DEV 1 5      # 网络接口
sar -q 1 5          # 运行队列与 load
sar -r 1 5          # 内存
sar -B 1 5          # 分页统计
```

**HFT：** 热路径机器 **sadc 间隔别太短**（如 ≥10s）；危机时用实时 `sar`，复盘用归档。

→ 字段详解：[附录 B sar 总结](./appendix-B-sar总结.md)

---

## 4.5 四大追踪器

Gregg 归纳的现代 Linux **高级追踪** 分工：

| 工具 | 定位 | 擅长 |
|------|------|------|
| **perf** | 官方剖析器 | CPU 采样、PMC、部分 trace、火焰图 |
| **Ftrace** | 内核内置 | 内核函数路径、调度、irq、latency histogram |
| **BCC** | eBPF + Python/Lua 前端 | 复杂脚本、生产级工具集（biolatency…） |
| **bpftrace** | eBPF 单行 DSL |  ad hoc 查询、一行命令、教程友好 |

**关系：**

```
        ┌─────────── 数据源 ───────────┐
        │ /proc  PMC  tracepoint     │
        │ kprobe  uprobe  USDT       │
        └─────────────┬──────────────┘
                      │
     ┌────────────────┼────────────────┐
     ▼                ▼                ▼
   perf            Ftrace          eBPF 引擎
     │                │                │
     │                │         ┌──────┴──────┐
     │                │         ▼             ▼
     └────────────────┴────  BCC        bpftrace
```

**HFT 实践路径：**

1. **perf** — 火焰图、cache miss（Ch 13）
2. **bpftrace** — syscall 计数、run queue 延迟、网络栈 tracepoint（Ch 15 + 附录 C）
3. **Ftrace** — 内核延迟 odd case（Ch 14）
4. **BCC** — 现成工具不够时再写 Python BPF

---

## 4.6 观测的观测（Observing Observability）

Gregg 提醒：**对观测结果保持怀疑**。

| 风险 | 说明 |
|------|------|
| **工具实现错误** | 解析 /proc 字段 bug、单位搞错 |
| **内核指标 bug** | 某版本 counter 不准（需对照多个来源） |
| **手册错误** | 官方文档与行为不一致 |
| **观测者效应** | 追踪本身占 CPU、改 cache 行为、拖慢被测系统 |

**应对：**

1. **交叉验证** — 同一现象用两种数据源（如 `vmstat` vs `/proc/stat` vs perf）
2. **控制变量** — 追踪开关前后对比，量化 overhead
3. **限范围/限时长** — 尤其生产 HFT
4. **记录工具版本** — perf/BPF 与内核必须匹配

**HFT：** 共置机上「为了 debug 开的全量 trace」本身可能改变 **P99** — 先在 shadow/replay 环境复现，再缩短生产窗口。

---

## 工具选型 · 对照 Ch 2 方法论

| Ch 2 方法 | 首选工具层 |
|-----------|------------|
| **USE（资源）** | vmstat, mpstat, sar, iostat, `/proc/net/dev` |
| **RED（服务）** | 自研 metrics + pidstat；网关可用 Prometheus |
| **延迟分解** | USDT/日志时间戳、bpftrace tracepoint、perf sched |
| **CPU 热点** | perf record → 火焰图 |
| **历史波动** | sar -f / sadc |
| **深度内核** | Ftrace / bpftrace |

---

## 危机响应 · 推荐顺序（HFT）

```
0. 工具已预装（4.1）
1. Ch 1 60 秒清单（uptime, vmstat, mpstat, pidstat, sar -n DEV…）
2. 定位 PID/核 → pidstat -t -p PID 1
3. CPU 高 → perf top / 短 perf record
4. 延迟尖刺 → bpftrace 一行（runqlat、syscall 计数）— 限 30s
5. 网络 → sar -n EDEV；必要时短 tcpdump
6. 需内核路径 → Ftrace / bpftrace tracepoint
7. 归档 sar + 记录工具版本，供事后「观测的观测」
```

---

## 本章学习目标 · 自检

- [ ] 能列出 **危机工具包** 应预装的包名
- [ ] 能区分 **固定计数器 / 剖析 / 追踪 / 监控** 的开销与细节
- [ ] 能区分 **系统级 vs 进程级** 工具选型
- [ ] 说清 **/proc、PMC、tracepoint、USDT、kprobe、uprobe** 各是什么
- [ ] 知道 **sar + sadc** 对历史分析的价值
- [ ] 能说明 **perf / Ftrace / BCC / bpftrace** 分工
- [ ] 理解 **观测者效应**，生产追踪会限时长

---

## HFT 精读捷径（Ch 4 在路线中的位置）

```
Ch 1  60 秒清单
Ch 2  USE / 延迟分解
Ch 3  内核与 syscall（理解数据源）
Ch 4  观测工具（本章：选型 + 数据源）
  → Ch 6/7/10 资源专章（带着工具读）
  → Ch 13 perf
  → Ch 14 Ftrace
  → Ch 15 BPF + 附录 C + 09-BPF
```

**本章最小行动集：**

1. 在 dev/裸机装好 **perf + bpftrace**，跑通 `perf stat` 与一条附录 C 脚本。
2. 配置 **sadc** 归档，练习 `sar -f` 读昨天 CPU/网络。
3. 对策略进程做一次 **99 Hz perf record → 火焰图**，对照 Ch 2 延迟分解。

---

## 相关章节

- 上一章：[chapter-03-操作系统.md](./chapter-03-操作系统.md)
- 下一章：[chapter-05-应用程序.md](./chapter-05-应用程序.md)
- perf：[chapter-13-perf性能分析.md](./chapter-13-perf性能分析.md)
- Ftrace：[chapter-14-Ftrace跟踪.md](./chapter-14-Ftrace跟踪.md)
- BPF：[chapter-15-BPF技术.md](./chapter-15-BPF技术.md)
- 附录 A USE：[appendix-A-USE方法Linux.md](./appendix-A-USE方法Linux.md)
- 附录 B sar：[appendix-B-sar总结.md](./appendix-B-sar总结.md)
- 附录 C bpftrace：[appendix-C-bpftrace单行命令.md](./appendix-C-bpftrace单行命令.md)
- BPF 专书：[09-BPF-Performance-Tools](../09-BPF-Performance-Tools/)
- 全书目录：[OUTLINE.md](./OUTLINE.md)
