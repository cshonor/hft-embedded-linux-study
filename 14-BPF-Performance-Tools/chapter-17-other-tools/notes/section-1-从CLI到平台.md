# 1. 从 CLI 到平台

| 层次 | 代表 | 适用 |
|------|------|------|
| **CLI 诊断** | BCC、bpftrace（Ch 4–5） | **Incident 深潜、短窗口** |
| **主机 GUI** | **Vector + PCP** | 单机实时热力图 |
| **企业面板** | **Grafana + PCP/Prometheus** | 历史趋势、告警 |
| **K8s 集群** | **kubectl-trace**、Cilium | 节点/Pod 级追踪 |

```
bpftrace/bcc（点查）
        ↓ 集成
PCP / ebpf_exporter → Grafana / Prometheus
        ↓
告警 · 热力图 · 集群 kubectl-trace
```

---
