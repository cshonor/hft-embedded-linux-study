## 16.1.5–16.1.6 PMC 与软件事件

### 性能监控计数器（PMCs）

当 **宏观统计** 无法解释「为何 CPU 上更快」— 下钻 **微架构**：

```bash
perf stat -e cycles,instructions,cache-misses,LLC-load-misses,branch-misses -- ...
# IPC = instructions / cycles ↑ ？
```

| PMC 信号 | 可能含义 |
|----------|----------|
| **IPC ↑** | 更少 stall — cache/分支/调度改善 |
| **cache-miss ↓** | 数据布局、NUMA、邻居争用减少 |
| **branch-miss ↓** | 热路径分支更可预测 |
| cycles ↓ 且 instructions 同 | **真变少** vs 频率变化 |

**案例精神：** 用 **硬件计数量化** 「快了多少、像哪类快」— 再对齐软件假设。

→ Ch 6 [IPC / PMC](../../chapter-06-cpus/) · Ch 13 [`perf stat`](../../chapter-13-perf/)

### 软件事件（Software Events）

**交叉验证** PMC 现象的内核/运行时原因：

| 事件 | 工具 | 关联 |
|------|------|------|
| `context-switches` / `cpu-migrations` | `perf stat` | 调度、绑核 |
| `page-faults` / `major-faults` | `perf stat` | 内存、Swap |
| sched tracepoint | perf / BPF | 迁移、run queue |
| syscalls | `perf trace` / BPF | 路径变短？ |

**组合阅读：**

```
IPC ↑ + context-switches ↓  →  调度/绑核故事
IPC ↑ + cache-miss ↓        →  布局/邻居/cache 故事
IPC 同 + 吞吐 ↑              →  可能 workload 变或并行度变
```

→ Ch 7 [缺页](../../chapter-07-memory/) · Ch 5 [线程状态](../../chapter-05-applications/)

---


---

← [本章导读](../README.md)
