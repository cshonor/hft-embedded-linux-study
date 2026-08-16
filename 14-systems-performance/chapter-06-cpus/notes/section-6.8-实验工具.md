## 6.8 实验工具 · CPU 基准

> ← [6.7 可视化](./section-6.6-6.7-观测工具与可视化.md) · [6.9 调优](./section-6.9-CPU-调优.md) · [本章导读](../README.md)

---

### 6.8.1 Ad Hoc

最简单的 CPU 满载 — 验证观测工具是否正常：

```bash
# 单线程 CPU-bound（"hot on one CPU"）
while :; do :; done &
# 用完 kill
```

**HFT 用法：** 验证 `mpstat -P ALL 1` 是否正确显示某核 100% usr — 工具可信才能信生产数据。

---

### 6.8.2 SysBench

CPU 微基准 — 计算质数：

```bash
sysbench --num-threads=8 --test=cpu --cpu-max-prime=100000 run
```

| 输出字段 | 含义 |
|----------|------|
| total time | 8 线程算完 100000 质数的总时间 |
| per-request min/avg/max | 单次事件延迟分布 |
| Threads fairness | 线程间负载均衡度（stddev 越小越好） |

**用途：** 不同系统/配置间 CPU 性能对比（前提：编译选项一致）。

详见 [Ch12 Benchmarking](../../chapter-12-benchmarking/)。

---

### HFT 视角

| 工具 | HFT 用途 |
|------|----------|
| Ad Hoc | 验证观测工具可信度 — 先确保工具对再信生产数据 |
| SysBench | 粗比不同 CPU/内核版本的 **纯算力** — 但 HFT 性能瓶颈不在算质数 |

**HFT 真正的 CPU 实验：** 行情回放压测（tick-to-trade 端到端）— 不是微基准算质数，而是 **真实工作负载 + 真实资源竞争**。详见 [Ch12 基准测试](../../chapter-12-benchmarking/)。

**操作建议：** 跑实验时 **始终开 `mpstat -P ALL 1`** — 确认 CPU 使用率和并行度符合预期。

 [Ch1.8 微观 vs 宏观](../../chapter-01-intro/notes/section-1.8-实验与微观宏观基准.md) · [Ch12](../../chapter-12-benchmarking/) · [HFT ch10 延迟测量](../../../16-hft-engineering/chapter-10-延迟测量与基准压测.md)


---

← [6.7 可视化](./section-6.6-6.7-观测工具与可视化.md) · [6.9 调优](./section-6.9-CPU-调优.md) · [本章导读](../README.md)
