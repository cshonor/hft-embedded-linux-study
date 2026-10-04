## 2.9 监控

> **路径：** [2.5 埋点](./section-2.5-性能分析方法论.md) → **Prometheus + Grafana 盯盘** → [2.10 可视化](./section-2.10-可视化.md) 抓 tail 根因。
> ← [2.8 统计](./section-2.8-统计.md) · [Ch1.7 观测四层](../../chapter-01-intro/notes/section-1.7-观测工具四层递进.md)

---

### 2.9.1 时间模式

监控系统记录 **时间序列** — 过去与现在对比，识别基于时间的使用模式。

| 周期 | 典型来源 | HFT 对应 |
|------|----------|----------|
| **小时级** | 监控/报表任务，5/10 分钟周期 | 定时风控报表、结算任务 |
| **日级** | 工作时间 9-5、日志轮转、备份 | **开盘/收盘** burst |
| **周级** | 工作日 vs 周末 | 周末休市 → 基线不同 |
| **季度级** | 财报 | 期货交割日/换月 |
| **年级** | 学校/假期 | 节假日流动性骤降 |
| **不规则** | 发版、促销、停电、体育决赛 | 版本上线、交易所维护 |

> Gregg 在 Netflix SRE 值班时学到：查社交媒体确认停电，问群聊是否有体育决赛。

---

### 2.9.2 监控产品

**工作原理：** agent/exporter 在目标系统上采集统计 → 要么执行 OS 工具（iostat/sar）解析输出（低效），要么直接读内核接口。

| 特性 | 说明 |
|------|------|
| **规模化** | 云环境数百/数千实例 → 必须集中监控 |
| **Netflix Atlas** | 20 万+实例，自研开源时序数据库 |
| **自定义 agent** | 针对 web server、DB、语言运行时 |

详见 [Ch4.2.4 Monitoring](../../chapter-04-observability-tools/)。

---

### Prometheus + Grafana（HFT 线上黄金组合）

**不用深挖原理，会用就行** — 云原生与性能监控事实标准。

```
C++/Rust 埋点 → Prometheus（时序库 + 拉取） → Grafana（面板）
```

| 角色 | 干什么 |
|------|--------|
| **Prometheus** | 存 **counter、histogram**（报单延迟、CPU、orders/s） |
| **Grafana** | 搭 **折线、散点、Heatmap** — 盯实盘 / 压测 |

**C++ / Rust 相同套路：**

| | C++ | Rust |
|--|-----|------|
| 计时 | `std::chrono::steady_clock` | `std::time::Instant` |
| 导出 | prometheus-cpp / HTTP `/metrics` | `metrics`、`prometheus` crate |
| 查询 | `histogram_quantile(0.99, rate(...))` | 同左 |

**Graphite：** Prometheus **之前** 的 TSDB；老量化机构可能仍用 — 逻辑大同小异，会 Prometheus 转过去很快。

---

### 2.9.3 启动以来汇总

如果没做持续监控，至少检查 **启动以来汇总值**（summary-since-boot）— 可与当前值对比。

**HFT：** 策略进程启动后应立即打 **基线快照**（cache miss 率、syscall 频率、IRQ 分布）— 等出事再装监控就晚了。

---

### 检查单

- [ ] **Prometheus + Grafana** 已接报单延迟、CPU、orders/s
- [ ] P99 独立告警（不与 mean 混）
- [ ] 监控覆盖 **开盘/收盘** burst 时段
- [ ] 策略进程启动即打基线快照


### 常见陷阱

1. 监控只看折线图——延迟分布需要直方图（histogram），折线图看不到分布形状和尾部
2. 告警阈值用绝对值——应该用相对基线（如 P99 > baseline * 1.5），绝对阈值不能适应负载变化
3. 不存历史数据——出事才想看历史趋势，Prometheus 默认 15 天可能不够，需要长期存储

<details>
<summary>自测题（点击展开）</summary>

1. 为什么延迟监控需要直方图而不是折线图？
   <details><summary>答</summary>直方图显示完整分布形状和尾部，折线图只显示某个聚合值（如 mean），看不到 P99 尖刺</details>
2. HFT 监控告警应该用什么阈值策略？
   <details><summary>答</summary>相对基线——如 P99 超过正常值 1.5 倍告警，而非绝对值（绝对值不适应负载变化）</details>
3. summary-since-boot 在 HFT 场景怎么用？
   <details><summary>答</summary>策略进程启动后立即打基线快照（cache miss、syscall 频率、IRQ 分布），出事后与当前对比</details>

</details>


---

← [2.8 统计](./section-2.8-统计.md) · [2.10 可视化](./section-2.10-可视化.md) · [本章导读](../README.md)
