# 3. 火焰图 (Flame Graphs)

Gregg 发明的 **栈 profile 可视化** — 把成千上万行栈折叠成一张图。

| 轴 | 含义 |
|----|------|
| **X 轴（宽度）** | 该栈路径 **样本占比** — 越宽 = CPU（或 off-CPU）时间越多 |
| **Y 轴（高度）** | **栈深度** — 底 = 根（如 `_start` / 内核入口），顶 = 叶子（实际干活的函数） |

**读法：** 找 **最宽的平台** — 即首要瓶颈路径；点击（交互版）可 zoom。

**与 BPF：** BCC `profile`、bpftrace `@[kstack]` / `stack()` 输出可喂给 `flamegraph.pl` — 与 `perf record` 火焰图 **同一套阅读逻辑**。

→ [SysPerf Ch 6 火焰图](../../../03-Systems-Performance-2nd/chapter-06-cpus/) · 工具 `stackcollapse` + `flamegraph.pl`

---
