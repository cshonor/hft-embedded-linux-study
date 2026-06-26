## 1.7 观测工具 · 四层递进

> 延迟指标 → [1.6](./section-1.6-延迟指标与读法.md) · 双视角 → [1.4](./section-1.4-热路径Resource与双视角.md)

### 观测 Observability — 四层递进：扫盲 → 趋势 → 热路径 → 拆单笔

Gregg 四类工具 = **从粗粒度统计到全链路追踪** 的排障阶梯；HFT 一般 **按顺序走，别跳层**。

```
计数器 Counters  →  指标 Metrics  →  剖析 Profiling  →  追踪 Tracing
   「有没有异常」     「什么时候开始高」   「哪段代码宽」      「这一笔逐步花在哪」
```

| 层 | 是什么 | 典型例子 | 回答什么 |
|----|--------|----------|----------|
| **① 计数器** | 最基础 **次数/累计** 统计 | 网卡收包数、`cache-misses`、softirq 次数、`ethtool -S` drop | 系统 **有没有异常波动**？resource 路况正常吗？ |
| **② 指标 Metrics** | **带时间维度** 的计数器/聚合 | 每秒收包延迟 histogram、P99 曲线、Prometheus 时序 | **什么时候开始高**？趋势还是尖刺？SLO 告警 |
| **③ 剖析 Profiling** | 一段时间内 **函数调用占比** 采样 | `perf record -g`、**火焰图** | **热路径哪段最宽**？平均/采样意义上的瓶颈 |
| **④ 追踪 Tracing** | **单个请求** 全链路耗时拆解 | eBPF、ftrace、分段 timestamp | **这一笔** P999/Max：RX→栈→策略→TX 各多少 μs？ |

#### 粒度：粗 → 中 → 细 — 追踪是 HFT 抓 tail 的核心

| 粒度 | 层 | 你看到什么 | 例句 |
|------|-----|------------|------|
| **粗** | 计数器 + 指标 | 聚合统计、趋势 | 「每秒 **平均** 延迟 10μs」「P999 曲线抬升了」 |
| **中** | 剖析 Profiling | **函数级** 时间占比 | 「**下单函数占 30% CPU**」— 不知 30% 里具体哪几步 |
| **细** | **追踪 Tracing** | **每一请求** 逐步耗时 + 内核事件 | 见下表 — **超过 P999 的 0.1% 卡在哪一步** |

**细粒度追踪 — 单笔订单拆段（例）：**

| 步骤 | 耗时 | 工具可见的更细事件 |
|------|------|-------------------|
| 网卡收包 | 2μs | DMA / softirq |
| 内核协议栈 | 3μs | copy、协议处理 |
| 策略计算 | 5μs | 解码 + 信号 |
| 网卡发包 | 1μs | TX queue |
| **（异常叠加）** | **+15μs** | **调度把线程换出** — 只有 trace 能看见 |

```
粗粒度：「延迟高了」
细粒度：「高在 IRQ 与进程抢同一核的那 12μs + 换出 15μs」→ 可改：迁 IRQ、isolcpus
```

**细粒度工具：`ftrace`、eBPF/bpftrace、perf trace**

- 把时间线拆到 **ms / μs / ns 级单个事件**（调度、锁、syscall、page fault）
- **意义：** 粗/中粒度只能报警；**追踪才能给出可落地的「哪一步、哪一类事件」**
- 专门服务：**P999 最慢 0.1%、Max 单次 spike** — Metrics 发现 tail → **Tracing 抓样本**

→ 深入：[Ch 14 ftrace](../../chapter-14-ftrace/) · [Ch 15 BPF](../../chapter-15-bpf/) · [04-BPF](../../../04-BPF-Performance-Tools/)

**HFT 排障顺序（固定套路）：**

```
1. 计数器 + 指标   →  确认「延迟确实高了 / P999 曲线抬升 / Max 告警」
2. 剖析           →  火焰图定位热路径、嫌疑函数
3. 追踪           →  把 **那一次** 长尾逐步拆透 → 可落地优化点
```

**和三类延迟指标的对应：**

| 延迟指标 | 主要靠哪几层 |
|----------|--------------|
| Mean / 压测基线 | 计数器 + Metrics |
| P99 / P999 | Metrics（histogram）+ Profiling（嫌疑） |
| **Max** | Metrics 捕获 + **Tracing 必做** |

| 类型 | 作用 | 后续章节 |
|------|------|----------|
| **计数器 Counters** | 累计事件（中断次数、包数） | Ch 4、Ch 6–10 |
| **指标 Metrics** | 时间序列、分位聚合 | 生产监控 |
| **剖析 Profiling** | 谁在耗 CPU — **火焰图** | Ch 13 perf、Ch 4 |
| **追踪 Tracing** | 单次时间线、因果 | Ch 14 Ftrace、Ch 15 BPF |

**HFT 命令速查：**

```bash
# ① 计数器 — 扫盲
mpstat -P ALL 1          ethtool -S eth0          perf stat -e cache-misses

# ② 指标 — 应用内 histogram / P99 曲线（或 Prometheus）

# ③ 剖析 — 热路径
perf record -g -p <pid> -- sleep 30 && perf report

# ④ 追踪 — 单笔 tail（示例方向，具体脚本见 Ch15）
# bpftrace / ftrace：订单 ID 关联 RX→decode→send 各段 Δt
```

→ [04-BPF Performance Tools](../../../04-BPF-Performance-Tools/) · [Ch 4 工具选型](../../chapter-04-observability-tools/) · [Ch 13 perf](../../chapter-13-perf/) · [Ch 14 ftrace](../../chapter-14-ftrace/) · 双视角排障 [1.4](./section-1.4-热路径Resource与双视角.md)
---

← [本章导读](../README.md)
