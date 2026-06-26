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

→ [12-HFT ch05](../../../15-HFT-Low-Latency-Practice/chapter-05-操作系统内核极致调优/)

### Cache、MMU、TLB、互连

| 组件 | 性能点 |
|------|--------|
| **L1/L2/L3** | 容量与延迟差数量级；**false sharing** 打穿一致性 |
| **MMU** | 虚拟地址翻译 |
| **TLB** | 页表缓存；miss 贵 — **大页（Huge pages）** 减 TLB 压力 |
| **QPI / UPI** | 多 socket 间互连 — **跨 socket 访存慢**，绑 NUMA 节点 |

→ [07-Gorman](../../../07-Linux-Virtual-Memory-Manager/) 页表 · [10-DPDK EAL](../../../14-DPDK-Low-Latency-Network/01-Intro-Book/notes/chapter-01-DPDK架构与EAL/) 大页

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

→ 深入 [Ch 13 perf](../../chapter-13-perf/) · [Ch 4 PMC 数据源](../../chapter-04-observability-tools/)

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

→ [05-LKD](../../../05-Linux-Kernel-Development/) 调度器 · Ch 3 [上下文切换](../../chapter-03-operating-systems/)

---


---

← [本章导读](../README.md)
