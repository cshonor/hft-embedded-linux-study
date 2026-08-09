# 1. 基础概念

| 术语 | 含义 |
|------|------|
| **BPF** | 经典 Berkeley Packet Filter — 最初用于 tcpdump 包过滤的 **内核字节码 VM** |
| **eBPF** | 扩展 BPF（2014+）— **通用、图灵完备、可验证** 的内核沙箱程序；本书「BPF」多指此 |
| **Tracing** | **事件追踪** — 每次事件发生记录一条（exec、open、syscall enter/exit） |
| **Snooping** | **嗅探** — 非修改地观察活动（opensnoop、execsnoop 一类） |
| **Sampling** | **采样** — 周期性快照（如 `profile` 按频率采栈）；低开销、可能漏短事件 |
| **Profiling** | **剖析** — 汇总「时间/次数花在哪」；常与采样栈或聚合 map 结合 |
| **Observability** | **可观测性** — 从外部输出推断内部状态；BPF 让 **内核 + 用户态** 同屏可见 |

> **HFT 直觉：** 延迟尖刺往往是 **短事件**（一次 block I/O、一次 run-queue 排队、一次 TCP 重传）— **Tracing/BCC 直方图** 补 **perf 采样** 的盲区；采样适合 CPU 热点，追踪适合「谁、何时、持续了多久」。


### 常见陷阱

1. **混淆 BPF 与 eBPF** — 经典 BPF 仅做包过滤（tcpdump），eBPF 是通用内核 VM；本书「BPF」默认指 eBPF，读到旧文档时要区分上下文
2. **以为 Sampling 能抓所有问题** — 采样按固定频率快照，短命进程和微秒级延迟尖刺可能完全漏掉；HFT 延迟排查应优先用 Tracing 而非 Sampling
3. **把 Observability 等同于 Logging** — 日志是应用主动输出，Observability 是从外部推断内部状态；BPF 的价值在于无需改代码即可透视内核+用户态

<details>
<summary>📝 自测题（点击展开）</summary>

1. **BPF 程序在哪一层运行？为什么这对性能很重要？**

   <details>
   <summary>参考答案</summary>

   BPF 程序在内核态运行（经过验证器检查安全性）。这意味着事件过滤、计数、聚合都在内核完成，只有最终结果通过 Map 送到用户态，避免了海量事件逐条拷贝到用户态的开销。

   </details>

2. **Tracing 和 Sampling 各自适合什么场景？**

   <details>
   <summary>参考答案</summary>

   Tracing 适合「谁、何时、持续了多久」的精确事件追踪（如每次 syscall 的延迟），能捕获短事件；Sampling 适合 CPU 热点定位（profile 按频率采栈），开销低但可能漏掉短命事件。HFT 延迟尖刺排查应优先 Tracing。

   </details>

3. **为什么说 BPF 让「内核 + 用户态」同屏可见？**

   <details>
   <summary>参考答案</summary>

   传统工具要么只看内核（/proc、perf），要么只看应用（日志、APM）。BPF 的 kprobe 看内核路径、uprobe 看用户态函数、USDT 看应用探针，可在同一工具链内关联跨层因果链。

   </details>

</details>

---
