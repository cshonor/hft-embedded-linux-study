## 6.5 性能分析方法论

### USE 方法（CPU）

对 **每个 CPU**（或每组 dedicated cores）：

| 字母 | CPU 上问什么 | 怎么量 |
|------|--------------|--------|
| **U** Utilization | 非 idle % | `mpstat -P ALL 1` |
| **S** Saturation | run queue、调度延迟 | `vmstat 1` 的 `r`；`runqlat`；**PSI cpu** |
| **E** Errors | 硬件错 | `mcelog`、EDAC、perf 不可代 |

→ [附录 A](../../appendix-A-USE方法Linux.md)

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


---

← [本章导读](../README.md)
