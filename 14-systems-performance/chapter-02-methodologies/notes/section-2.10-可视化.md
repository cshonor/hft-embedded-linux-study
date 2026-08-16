## 2.10 可视化

> 可视化允许检查比文本更容易理解（甚至显示）的更多数据 — 模式识别与模式匹配。
> ← [2.9 监控](./section-2.9-监控.md) · [2.8 统计](./section-2.8-统计.md)

---

### 2.10.1 折线图 Line Chart

最基础的可视化 — X 轴时间，适合 **趋势** 和 **尖峰** 识别。

**进阶用法：** 同图绘多条线（如每个磁盘、或 median/mean/σ/P99），可比比各组件表现。

> Gregg 示例：MySQL 磁盘 I/O 延迟均值 ~4ms 看似正常，但加入 P99 后发现 1% I/O >20ms — mean 隐藏了尾部。

**HFT：** 盘前 30 min P99 + CPU 一眼尖峰。工具：**Grafana** Time series ← Prometheus。

---

### 2.10.2 散点图 Scatter Plot

每个事件画一个点（X=时间，Y=延迟）— **看到全部数据**。

**优势：** 暴露 outlier — 折线图看不到的 >50ms 尖刺在散点图上一目了然。

**劣势：** 数据量大时点重叠（"wall of paint"）；采集/存储开销大（每个 I/O 的 x,y 坐标）。

**HFT：** λ vs P99 散点 → [限流 / ρ 预警](./section-2.6.2-M-M-1-拐点与预警线.md)。工具：Grafana XY / Python Seaborn。

---

### 2.10.3 热力图 Heat Map

> Gregg 发明了计算领域中 **延迟热力图** 的应用（2008 年 Sun ZFS Storage Appliance 首次发布）。

**原理：** 将 X/Y 范围量化为 bucket，用颜色深度表示事件密度 — 解决散点图的可扩展性问题。

| 优势 | 说明 |
|------|------|
| **可扩展** | 单机到数千机同一可视化 |
| **模式发现** | 双峰分布、周期性抖动在热力图上一目了然 |
| **离群值** | 高位浅色块 = 高延迟 outlier |

> Gregg 示例：磁盘 I/O 热力图显示双峰 — 近零延迟（cache hit）+ ~1ms（cache miss）。

**HFT：** 时间 × 延迟分桶 — GC/IRQ/开盘 burst 规律。工具：Grafana **Heatmap**（Prometheus `_bucket`）；Seaborn；`perf heatmap`。

详见 [Ch6.7 Visualizations](../../chapter-06-cpus/)、[Ch9.7.3 Latency Heat Maps](../../chapter-09-disks/)。

---

### 2.10.4 时间线图 Timeline Chart

一组活动以 **条形** 绘在时间线上 — 前端性能分析中称 **瀑布图 Waterfall**。

- **前端：** 浏览器网络请求时序，含依赖关系（Gantt chart 变体）
- **后端：** 线程/CPU 时序，如 **KernelShark**、**Trace Compass**（后者画唤醒依赖箭头）

**HFT：** 单笔订单 RX→decode→strategy→TX 各段时序 — 用 ftrace/bpftrace 生成 → Trace Compass 或自写脚本可视化。

---

### 2.10.5 表面图 Surface Plot

三维数据渲染为 **三维表面**（常为线框模型）— 适合第三维变化不剧烈的场景。

> Gregg 示例：数据中心 300+ 台服务器、5312 CPU 的利用率表面图。每台 16 CPU 为行、60 秒为列、高度=利用率。棋盘格布局中可看出哪些服务器持续 100%。

可扩展到 6 维： hue（第 4 维）+ saturation（第 5 维）+ pattern（第 6 维）。

---

### 2.10.6 可视化工具

Unix 性能分析历史聚焦 **文本工具** — 快速、实时、login session 可用。可视化则需 trace-and-report 周期，紧急时太慢。

现代工具（Grafana 等）通过浏览器/移动端提供 **实时视图**，缩小了差距。

**HFT 线上/线下分工：**

| 场景 | 工具 | 用途 |
|------|------|------|
| **线上盯盘** | Grafana 折线 + Heatmap | Prometheus 喂数 |
| **线下热点** | perf 火焰图 | CPU 烧在哪 |
| **偶发 tail** | FlameScope | 框选异常窗口对比栈 |

```bash
# 火焰图（C++）
perf record -F 99 -p $(pidof gateway) -g -- sleep 30
perf script | stackcollapse-perf.pl | flamegraph.pl > flame.svg

# Rust
cargo flamegraph --bin gateway

# FlameScope — 长时间采样后网页框选
perf record -F 49 -p $(pidof gateway) -g -- sleep 300
# → Netflix/flamescope 导入 perf.data
```

 [Ch 13 perf](../../chapter-13-perf/) · [Ch 15 BPF](../../chapter-15-bpf/) · [Ch6.7 火焰图](../../chapter-06-cpus/)

---

### 选型速查

| 你想问… | 图 | 工具 |
|---------|-----|------|
| P99 刚尖了？ | 折线 | **Grafana** |
| 限流设多少？ | 散点 | Grafana / Python |
| 每分钟规律抖？ | 热力 | Grafana / Seaborn |
| 谁吃 CPU？ | 火焰 | perf / cargo flamegraph |
| 偶发 2 ms 谁干的？ | FlameScope | perf + 网页 |
| 单笔各段多久？ | 时间线 | ftrace + Trace Compass |


### 常见陷阱

1. 用折线图看延迟分布——折线只显示聚合值，看不到双峰和尾部，必须用 histogram/热力图
2. 散点图数据量太大——百万级事件点重叠成"墙"，应改用热力图（列量化）
3. 忽视可视化工具自身开销——长时间 perf record 会产生大量数据，影响被测系统

<details>
<summary>自测题（点击展开）</summary>

1. 热力图相比散点图有什么优势？
   <details><summary>答</summary>量化 bucket 解决点重叠问题，可扩展到数千台机器同一可视化，还能发现周期性模式</details>
2. FlameScope 解决什么问题？
   <details><summary>答</summary>长时间 perf record 后，网页框选 P99 飙高窗口，对比异常时段与正常时段的调用栈差异</details>
3. 表面图适合什么数据？
   <details><summary>答</summary>三维数据且第三维变化不剧烈——如数据中心多服务器多 CPU 的利用率随时间变化</details>

</details>


---

← [2.9 监控](./section-2.9-监控.md) · [本章导读](../README.md)
