# 3. 火焰图 (Flame Graphs)

Gregg 发明的 **栈 profile 可视化** — 把成千上万行栈折叠成一张图。

| 轴 | 含义 |
|----|------|
| **X 轴（宽度）** | 该栈路径 **样本占比** — 越宽 = CPU（或 off-CPU）时间越多 |
| **Y 轴（高度）** | **栈深度** — 底 = 根（如 `_start` / 内核入口），顶 = 叶子（实际干活的函数） |

**读法：** 找 **最宽的平台** — 即首要瓶颈路径；点击（交互版）可 zoom。

**与 BPF：** BCC `profile`、bpftrace `@[kstack]` / `stack()` 输出可喂给 `flamegraph.pl` — 与 `perf record` 火焰图 **同一套阅读逻辑**。

→ [SysPerf Ch 6 火焰图](../../../14-systems-performance/chapter-06-cpus/) · 工具 `stackcollapse` + `flamegraph.pl`


### 常见陷阱

1. **把火焰图当作 profile 的替代品** — 火焰图是 profile/stackcount 的可视化形式，不是独立数据源；先确保数据采集正确（正确的 probe、足够的采样时长），再画图
2. **误读火焰图的宽度** — 火焰图横向宽度 = 该函数在采样中出现的比例，不代表单次执行时间；宽的函数可能因为被调用次数多而非单次慢
3. **忽视采样频率的选择** — 默认 99Hz 是为了避免与定时器事件共振；HFT 场景可能需要更高频率，但频率越高开销越大

<details>
<summary>📝 自测题（点击展开）</summary>

1. **火焰图的横轴和纵轴分别代表什么？**

   <details>
   <summary>参考答案</summary>

   纵轴 = 调用栈深度（底部是调用者，顶部是被调用者），横轴 = 该函数在所有采样中出现的比例（不是时间轴）。一个「宽」的顶部条表示该函数占用了较多 CPU 采样。

   </details>

2. **为什么采样频率常用 99Hz 而非 100Hz？**

   <details>
   <summary>参考答案</summary>

   99 是质数，避免与系统中其他 100Hz/1000Hz 的周期性事件（如定时器、心跳）产生共振效应，导致采样总是命中同一相位。99Hz 让采样点均匀散布在不同执行阶段。

   </details>

3. **HFT 场景中火焰图有什么局限性？**

   <details>
   <summary>参考答案</summary>

   (1) 采样可能漏掉微秒级延迟尖刺；(2) 只显示 on-CPU 时间，off-CPU 等待不显示（需 off-CPU 火焰图）；(3) 宽度代表频次不是延迟，宽函数可能是高频低延迟调用。HFT 应结合 offcputime 和直方图使用。

   </details>

</details>

---
