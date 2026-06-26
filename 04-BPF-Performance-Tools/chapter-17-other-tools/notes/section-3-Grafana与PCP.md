# 3. Grafana 与 PCP

**Grafana** — 主流开源 **Dashboard** 工具。

### 两种 PCP 数据源模式

| 模式 | 插件/路径 | 特点 |
|------|-----------|------|
| **实时 (live)** | **grafana-pcp-live** | 轮询 PCP **最新** 指标；浏览器保留短历史 |
| **历史 (redis)** | **grafana-pcp-redis** | PCP → **Redis** 持久化 — 长期趋势 |

| live | redis |
|------|-------|
| **无人看时开销低** | 适合 **容量规划、回归对比** |
| 适合 **当前主机 deep dive** | 存历史 baseline |

### 面板示例

Grafana **Heatmap** 面板展示 **`bcc.runq.latency`** 等 — 与 Vector 热力图同族。

**HFT：** Grafana 存 **runqlat P99 桶、tcpretrans 计数** 等 **少量 SLI** — 非全量 BCC 工具集。

---
