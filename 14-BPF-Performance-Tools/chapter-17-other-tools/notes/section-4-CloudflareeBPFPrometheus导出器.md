# 4. Cloudflare eBPF Prometheus 导出器

### Prometheus 生态

| 组件 | 角色 |
|------|------|
| **Prometheus** | 拉取 (pull) 指标、存储、告警 |
| **Grafana** | 查询 Prometheus 画图 |
| **K8s** | ServiceMonitor 等原生集成 |

### `ebpf_exporter`

[Cloudflare ebpf_exporter](https://github.com/cloudflare/ebpf_exporter) — 将 **eBPF 程序** 采集的数据转为 **Prometheus 格式** 暴露 `/metrics`。

| 优点 | 注意 |
|------|------|
| 标准 **PromQL**、Alertmanager | Exporter 本身也跑 BPF — **需控频率** |
| 与 K8s/Grafana 栈统一 | 指标设计要 **聚合**，非 per-event |

**HFT 示例指标（概念）：**

- `runqlat` 直方图 bucket → histogram metric  
- `tcpretrans` counter  
- 与 **应用 tick-to-trade histogram** 同屏 — 区分 **内核 vs 策略**

---
