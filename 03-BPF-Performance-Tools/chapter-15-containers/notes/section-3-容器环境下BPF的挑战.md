# 3. 容器环境下 BPF 的挑战

### 权限：在宿主机跑

| 事实 | 做法 |
|------|------|
| BPF 加载通常需 **root/CAP_BPF** | 在 **Host** 跑 BCC/bpftrace |
| 容器内常无权限 | Sidecar 观测 ≠ 容器内 trace |

**HFT：** 观测 agent 以 **DaemonSet on host** 或 **裸金属运维机** 执行 — 与策略容器隔离。

### 内核无统一「容器 ID」

内核只见 **cgroups + namespaces** — 没有 Docker ID 字段。

| 变通 | 用法 |
|------|------|
| **PID Namespace ID** | BPF 输出带 pidns → 按容器过滤 |
| **UTS nodename** | 读 hostname（常为容器名/ID） |
| **cgroup path** | `/sys/fs/cgroup/...` 映射到 pod |

### 过滤技巧

```bash
# 示意 — 以 man 为准
sudo runqlat-bpfcc --pidnss 10
sudo pidnss-bpfcc
```

在输出中按 **pidns / comm / cgroup** 筛特定 workload。

---
