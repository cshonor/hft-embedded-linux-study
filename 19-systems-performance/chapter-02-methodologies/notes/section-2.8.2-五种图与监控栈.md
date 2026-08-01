## 2.8.2 · 五种图与监控栈

> **路径：** [2.5 埋点](./section-2.5-性能分析方法论.md) → **Prometheus + Grafana 盯盘** → **perf / FlameScope 抓 tail 根因**。  
> ← [2.8.1 统计陷阱](./section-2.8.1-统计陷阱.md) · [Ch1.7 观测四层](../chapter-01-intro/notes/section-1.7-观测工具四层递进.md)

| 图 | 回答什么 | 场景 |
|----|----------|------|
| **折线** | 随时间变没变？尖峰？ | 盯盘、压测 |
| **散点** | 负载 vs 延迟拐点？ | 定限流 |
| **热力** | 延迟何时聚集？ | 周期抖动 |
| **火焰** | CPU 烧在哪？ | 热点优化 |
| **FlameScope** | 哪几秒异常？栈差？ | 偶发 tail |

---

### Prometheus + Grafana（线上黄金组合）

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

**Graphite：** Prometheus **之前** 的 TSDB；老量化机构可能仍用 — **逻辑大同小异**（指标名 + 时间序列 + 面板），会 Prometheus **转过去很快**。

---

#### ① 折线图

- 盘前 **30 min**：**P99、CPU** — 一眼尖峰
- 工具：**Grafana** Time series ← Prometheus

#### ② 散点图

- **λ vs P99** → [限流 / ρ 预警](./section-2.6.2-M-M-1-拐点与预警线.md)
- 工具：Grafana XY / Python **Seaborn** scatter

#### ③ 热力图

- **时间 × 延迟分桶** — GC/IRQ/开盘 burst 规律
- 工具：Grafana **Heatmap**（Prometheus `_bucket`）；Seaborn；`perf heatmap`

#### ④ 火焰图

```bash
# C++
perf record -F 99 -p $(pidof gateway) -g -- sleep 30
perf script | stackcollapse-perf.pl | flamegraph.pl > flame.svg

# Rust
cargo flamegraph --bin gateway
```

→ [Ch 13 perf](../../chapter-13-perf/)

#### ⑤ FlameScope

- 长 `perf record` → 网页 **框选 P99 飙高窗口** → 对比栈
- 对付 [2.8.1 陷阱③](./section-2.8.1-统计陷阱.md#陷阱-③)

```bash
perf record -F 49 -p $(pidof gateway) -g -- sleep 300
# → Netflix/flamescope 导入 perf.data
```

---

### 选型速查

| 你想问… | 图 | 工具 |
|---------|-----|------|
| P99 刚尖了？ | 折线 | **Grafana** |
| 限流设多少？ | 散点 | Grafana / Python |
| 每分钟规律抖？ | 热力 | Grafana / Seaborn |
| 谁吃 CPU？ | 火焰 | perf / cargo flamegraph |
| 偶发 2 ms 谁干的？ | FlameScope | perf + 网页 |

**线上：** Grafana 折线 + Heatmap（**Prometheus** 喂数）  
**线下：** 火焰图 → FlameScope

→ [Ch 13](../../chapter-13-perf/) · [Ch 15 BPF](../../chapter-15-bpf/)

---

### 与全书衔接

| 章节 | 关系 |
|------|------|
| [2.5 埋点](./section-2.5-性能分析方法论.md) | 数据进 Prometheus |
| [2.8.1 陷阱](./section-2.8.1-统计陷阱.md) | 面板必看 P99 非 mean |
| [2.7.2 SLO](./section-2.7.2-容量规划三步法.md) | 压测验收看 Grafana |

---

### 检查单

- [ ] **Prometheus + Grafana** 已接报单延迟、CPU、orders/s
- [ ] 折线 + Heatmap 上线；P99 独立告警
- [ ] 压测出 **散点** 定限流
- [ ] 线下 **perf 火焰图**；偶发 tail 用 **FlameScope**

---

← [2.8.1 统计陷阱](./section-2.8.1-统计陷阱.md) · [本章导读](../README.md)
