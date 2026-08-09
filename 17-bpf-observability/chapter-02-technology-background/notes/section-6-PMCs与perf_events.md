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


### 常见陷阱

1. **混淆 PMC 计数模式和采样模式** — 计数模式只累计事件总数，采样模式在计数溢出时中断记录 IP+栈；分析热点需要采样模式，看趋势用计数模式
2. **忽视 PMI 的 skid 问题** — 性能监控中断（PMI）不是精确的——从事件发生到中断响应有若干指令的滑移，记录的 IP 可能不是真正触发事件的指令；用 PEBS 缓解
3. **在虚拟化环境中期望 PMC 可用** — 云 VM 通常无法直接访问 PMC（需要直通或 vPMU 支持）；HFT 如果跑在 VM 里，PMC 分析可能不可用

<details>
<summary>📝 自测题（点击展开）</summary>

1. **PMC 的计数模式和溢出采样模式有什么区别？**

   <details>
   <summary>参考答案</summary>

   计数模式：累计硬件事件总数（如 L3 miss 总次数），适合看宏观趋势。溢出采样模式：计数到阈值后触发 PMI 中断，记录 IP 和栈，适合定位热点代码。`perf stat` 用计数模式，`perf record` 用采样模式。

   </details>

2. **什么是 PMI skid？PEBS 如何解决？**

   <details>
   <summary>参考答案</summary>

   PMI（性能监控中断）有延迟——从硬件事件触发到 CPU 响应中断，期间会执行若干条指令（skid），导致记录的 IP 偏离真正触发事件的指令。PEBS（Intel）用硬件缓冲区在事件发生时立即保存处理器状态，大幅减少 skid，适合微架构级分析。

   </details>

3. **BPF 如何与 PMC/perf_events 结合？**

   <details>
   <summary>参考答案</summary>

   BPF 程序可附加到 perf_event（`BPF_PROG_TYPE_PERF_EVENT`），在 PMC 溢出时执行 BPF 逻辑而非传统 perf 中断处理。这样可以在溢出时用 BPF Map 收集栈、关联上下文，比纯 perf 更灵活。日常 HFT 更多直接用 `perf record` + BCC `profile`。

   </details>

</details>

---
