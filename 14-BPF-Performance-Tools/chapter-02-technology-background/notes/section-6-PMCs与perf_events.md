# 6. PMCs 与 perf_events

### PMC（Performance Monitoring Counters）

| 模式 | 行为 |
|------|------|
| **计数** | 累计某硬件事件（L3 miss、分支误预测、指令退休…） |
| **溢出采样** | 计数到阈值 → 中断 → 记录 **IP +（可选）栈** — `perf record` 基础 |

### PEBS（Intel Precise Event-Based Sampling）

**问题：** 普通 PMI 中断有 ** skid ** — 记录的 IP 不是真正触发事件的那条指令。  
**PEBS：** 硬件 **更精确** 地关联事件与指令指针 — 微架构级分析（cache、内存延迟）时重要。

**与 BPF：** BPF 可 **附加在 perf_event** 上（`BPF_PROG_TYPE_PERF_EVENT`）— 把 PMC 溢出与 map/栈收集结合；日常 HFT 更多直接用 `perf` + BCC `profile`，PMC 细节见 [chapter-06-CPU.md](../../chapter-06-cpus/)。

---
