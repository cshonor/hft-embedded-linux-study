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


### 常见陷阱

1. 混淆计数器和采样——计数器是精确总计（低开销），采样是定时快照（有统计误差）
2. 追踪当采样用——追踪记录每个事件（高开销），不是采样子集，高频事件会打爆
3. 不区分系统级和进程级工具——mpstat 看全局但看不到具体进程，pidstat 才能定位到 TID

<details>
<summary>自测题（点击展开）</summary>

1. 观测工具按原理分哪几类？
   <details><summary>答</summary>计数器（/proc/stat）、采样（perf record）、追踪（strace/ftrace）、剖析（火焰图）</details>
2. 计数器和采样的根本区别？
   <details><summary>答</summary>计数器统计事件总次数（精确、低开销），采样定时取快照（有统计误差但开销可控）</details>
3. 为什么高频事件不适合逐条追踪？
   <details><summary>答</summary>每个事件都记录会产生大量 CPU/IO 开销，应该用 BPF map 聚合（直方图/计数）</details>

</details>


---

← [本章导读](../README.md)
