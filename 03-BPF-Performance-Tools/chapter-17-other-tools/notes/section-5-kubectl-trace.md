# 5. kubectl-trace

| 问题 | 解决 |
|------|------|
| BPF 习惯 **SSH 单机** | K8s 多节点 **不便** |
| **kubectl-trace** | 在 **集群节点/Pod/Container** 上跑 **bpftrace 脚本** |

```bash
# 示意 — 以项目文档为准
kubectl trace run node/<node-ip> -f vfsstat.bt
kubectl trace run pod/<pod-name> -f tcpretrans.bt
```

| 目标 | 用途 |
|------|------|
| **Node** | 宿主机级（同 Ch 15 Host 视角） |
| **Pod/Container** | 针对 workload — 仍受 **权限/namespace** 约束 |

**HFT：** **K8s 上的风控/网关 Pod** incident — 无需 SSH 逐节点；**策略裸金属** 不用。

→ [Ch 15 容器](../../chapter-15-containers/) · [Ch 5 bpftrace](../../chapter-05-bpftrace/)

---
