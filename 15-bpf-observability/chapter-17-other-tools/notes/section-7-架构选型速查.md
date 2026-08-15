# 7. 架构选型速查

| 需求 | 推荐栈 |
|------|--------|
| Incident 5 分钟点查 | **bcc/bpftrace CLI**（Ch 3–16） |
| 单机实时热力图 | **Vector + PCP + BCC PMDA** |
| 团队 Dashboard + 历史 | **Grafana + PCP-redis** 或 **Prometheus + ebpf_exporter** |
| K8s 节点跑 bpftrace | **kubectl-trace** |
| K8s 网络策略 | **Cilium** |
| 嵌入式 | **ply** |

---
