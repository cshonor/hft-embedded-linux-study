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

**解读：** IPC 低 + cache-misses 高 → 内存/布局问题；branch-misses 高 → 热分支。→ [Ch 6 CPU](../../chapter-06-cpus/)

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

→ 深入 BPF：[Ch 15](../../chapter-15-bpf/) · [04-BPF-Performance-Tools](../../../04-BPF-Performance-Tools/)

---


---

← [本章导读](../README.md)
