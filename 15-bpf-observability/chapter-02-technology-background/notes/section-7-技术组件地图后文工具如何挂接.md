# 7. 技术组件地图（后文工具如何挂接）

```
                    ┌─────────────────────────────────┐
                    │  eBPF VM + 验证器 + Map + Helper │
                    └───────────────┬─────────────────┘
                                    │
     ┌──────────────┬───────────────┼───────────────┬──────────────┐
     ▼              ▼               ▼               ▼              ▼
 kprobes       uprobes        Tracepoints         USDT      perf_event/PMC
 (内核动态)    (用户动态)      (内核静态)        (用户静态)    (硬件采样)
     │              │               │               │              │
     └──────────────┴─────── BCC / bpftrace 工具 ──┴──────────────┘
                                    │
                          聚合 map / 栈 ID → 用户态展示
                                    │
                          火焰图 / 直方图 / 文本流
```


### 常见陷阱

1. **把技术组件当作独立工具** — kprobes、tracepoints、PMCs 是底层机制，BCC/bpftrace 是前端；工具（biolatency、runqlat）是前端对底层机制的封装组合
2. **忽视组件间的依赖关系** — 如火焰图依赖 stackid，stackid 依赖 kprobe/uprobe/tracepoint；理解依赖链有助于排查「为什么工具没输出」
3. **选工具时忽视事件频率** — 高频事件应用 Map 聚合工具（funccount/argdist），低频事件可用逐行打印工具（trace）；选错工具要么漏数据要么拖慢系统

<details>
<summary>📝 自测题（点击展开）</summary>

1. **BPF 性能工具的技术栈从底到顶有哪几层？**

   <details>
   <summary>参考答案</summary>

   (1) 硬件层：PMC、PEBS；(2) 内核插桩层：kprobes、tracepoints、perf_events；(3) 用户态插桩层：uprobes、USDT；(4) BPF 核心层：verifier、JIT、Map、helper；(5) 前端层：BCC、bpftrace；(6) 工具层：biolatency、runqlat 等具体工具。

   </details>

2. **当 BPF 工具没有输出时，如何沿技术栈排查？**

   <details>
   <summary>参考答案</summary>

   从顶到底：(1) 前端语法是否正确？(bpftrace -d 看编译结果)；(2) probe 是否匹配到目标？(bpftrace -l 查)；(3) BPF 程序是否加载成功？(bpftool prog list)；(4) 目标事件是否真的触发？(用 strace/perf 先验证)；(5) 权限是否足够？(root/cap_bpf)。

   </details>

3. **HFT 场景中，如何根据事件频率选择技术组件？**

   <details>
   <summary>参考答案</summary>

   高频事件（如每包 recv）：用 tracepoint + Map 聚合，避免 per-hit 打印。中频事件（如 syscall enter）：可用 bpftrace 聚合 + 定时输出。低频事件（如进程创建）：可用 trace 逐行打印。PMC 溢出采样适合 CPU 热点定位，不适合短延迟事件。

   </details>

</details>

---
