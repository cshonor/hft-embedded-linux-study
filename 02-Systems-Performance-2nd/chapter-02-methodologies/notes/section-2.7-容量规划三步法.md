## 2.7 · 容量规划三步法

> **模型 → 压测 → 实盘** — 别等出问题才被动优化。  
> ← [2.6 排队论](./section-2.6-排队论概览与Kendall记号.md) · [Amdahl/USL](./section-2.7-阿姆达尔与USL.md) · [计算器](./section-2.6-排队论计算器.md)

---

### 三步闭环

```
① 模型算理论值     M/M/1·c 算 ρ、W、knee；Amdahl/USL 算加核与 2× tick
       ↓
② 压测 / 回放验证   扫 λ 找 knee；拟合 USL；验证网卡 pps
       ↓
③ 实盘迭代          Grafana：ticks/s、ρ、P99、drop — 校正 λ、μ、α、β
```

| 阶段 | 输入 | 输出 |
|------|------|------|
| **① 理论** | SLO、μ、f | 限流阈值、最少 c、网卡/内存量级 |
| **② 压测** | trace、阶梯 λ | 实测 knee、P99@ρ、USL 曲线 |
| **③ 实盘** | Prometheus | 修正模型；旺季前扩容/限流 |

**HFT 与 Auto Scaling：**

- **共置低延迟：** 固定容量 + headroom（告警低于拐点 10~15%），非临时扩容。
- **回放/离线风控：** 可弹性扩缩。

---

### 全书衔接

| 主题 | 章节 |
|------|------|
| 压测 | [Ch 12](../../chapter-12-benchmarking/) |
| CPU / false sharing | [Ch 6](../../chapter-06-cpus/) |
| 内存 WSS | [Ch 7.4](../../chapter-07-memory/notes/section-7.4-分析方法论.md) |
| 网卡 pps | [Ch 10](../../chapter-10-network/) |
| Workload 先 | [2.4](./section-2.4-两种分析视角.md) |

---

### 检查单

- [ ] **预警线**：M/M/1 **70%**、M/D/1 **80%**、M/M/8 **75~80%**
- [ ] **告警**：比拐点低 **10~15%**（M/M/1 → **~60%**）
- [ ] **计算器**：c、λ、1/μ → W、P99、knee
- [ ] **Amdahl**：f>10% 先减串行再加核
- [ ] **USL**：α、β — 是否过扩展峰值
- [ ] tick **2×**：核数、pps、RSS + headroom
- [ ] **Ch12 压测 → Grafana** 闭环

---

← [USL](./section-2.7-阿姆达尔与USL.md) · [2.8 统计与可视化](./section-2.8-2.10-统计与可视化.md) · [本章导读](../README.md)
