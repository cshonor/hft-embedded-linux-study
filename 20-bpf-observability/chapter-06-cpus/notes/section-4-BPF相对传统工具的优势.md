# 4. BPF 相对传统工具的优势

| 盲区 | BPF 如何补 |
|------|------------|
| **极短命进程** | `top` 采样不到 → `execsnoop` |
| **运行队列等待** | `mpstat` 只见忙闲 → **`runqlat` 直方图** |
| **Off-CPU 原因** | `perf` 默认 on-CPU → **`offcputime`** |
| **按进程 LLC** | `perf stat` 粗粒度 → **`llcstat`** |


### 常见陷阱

1. **以为 BPF 完全替代传统工具** — BPF 补的是传统工具的盲区，不是替代；top/mpstat 做快速概览仍然有用，BPF 做深度钻取
2. **忽视 BPF 工具的开销差异** — BPF 工具开销从低到高：funccount(Map 聚合) < profile(采样) < trace(逐行) < stackcount(栈聚合)；选错工具会放大开销
3. **在不需要精确度时用 BPF 重工具** — 如果 top 已经能看出问题（如某进程 100% CPU），不需要用 BPF trace 逐行追踪；先用轻工具确认范围再用重工具钻取

<details>
<summary>📝 自测题（点击展开）</summary>

1. **BPF 相比传统工具能补哪些盲区？**

   <details>
   <summary>参考答案</summary>

   (1) 极短命进程：top 采样不到→execsnoop 逐事件追踪；(2) 运行队列延迟：mpstat 只见忙闲→runqlat 直方图看排队时间分布；(3) off-CPU 原因：perf 默认 on-CPU→offcputime 看阻塞原因；(4) per-进程 LLC：perf stat 粗粒度→llcstat 精确计数；(5) 任意函数级追踪：传统工具做不到→bpftrace kprobe/uprobe。

   </details>

2. **BPF 工具的开销排序是什么？HFT 如何选择？**

   <details>
   <summary>参考答案</summary>

   从低到高：(1) funccount/argdist（Map 聚合，per-hit 开销极低）；(2) profile:hz:99（定时采样，固定开销）；(3) stackcount（栈聚合，per-hit 有 stackid 开销）；(4) trace（逐行打印，per-hit 有 ring buffer 开销）。HFT 原则：热路径用 Map 聚合工具，冷路径可用 trace；所有工具用完即撤。

   </details>

3. **什么时候应该用传统工具而非 BPF？**

   <details>
   <summary>参考答案</summary>

   (1) 快速概览系统状态——top/mpstat 秒级出结果，BCC 工具有编译延迟；(2) 确认问题是否存在——如果 top 已显示某进程 100% CPU，不需要 BPF；(3) 长期监控——传统工具开销固定且可预测，BPF 工具不适合长期挂载；(4) 无 root 权限——传统工具普通用户可用，BPF 需要 root。

   </details>

</details>

---
