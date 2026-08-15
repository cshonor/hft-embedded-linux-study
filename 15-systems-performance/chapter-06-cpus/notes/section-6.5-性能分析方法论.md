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


### 常见陷阱

1. USE 只查 Utilization——Saturation（run queue/调度延迟）才是 HFT 的关键指标
2. profiling 不加帧指针——perf record -g 需要帧指针，编译去掉后栈全是 [unknown]
3. 只看全局 CPU——HFT 热路径在特定核上，全局平均正常但热核可能已饱和

<details>
<summary>自测题（点击展开）</summary>

1. CPU 的 USE 方法中 HFT 最该关注哪个字母？
   <details><summary>答</summary>Saturation——run queue 长度和调度延迟，HFT 延迟尖刺多因调度等待而非 CPU 不够</details>
2. perf record 剖析的前置条件是什么？
   <details><summary>答</summary>编译保留帧指针（-fno-omit-frame-pointer）——否则栈回溯全是 [unknown]</details>
3. 为什么 HFT 要看 per-CPU 而不是全局 CPU？
   <details><summary>答</summary>热路径绑在特定核上——全局平均可能正常，但热核已 100% + run queue 堆积</details>

</details>


---

← [本章导读](../README.md)
