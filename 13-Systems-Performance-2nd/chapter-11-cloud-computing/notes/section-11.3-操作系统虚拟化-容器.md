## 11.3 操作系统虚拟化 / 容器

### Namespaces + cgroups

| 机制 | 隔离什么 |
|------|----------|
| **Namespaces** | PID、NET、MNT、UTS、IPC、USER — **视图隔离** |
| **cgroups** | CPU、内存、blkio、pid 数 — **资源限与统计** |

容器 **不是** 内核里单一对象 — 是 **ns + cgroup 的组合**（Docker/LXC/containerd）。

### 开销与争用

| 方面 | 容器 vs VM |
|------|------------|
| **CPU/内存映射** | 容器 **几乎无额外层** — 同一内核 |
| **启动速度** | 毫秒级 |
| **主要问题** | **共享内核 + 共享物理 cache/TLB/锁** — noisy neighbor |

**HFT 启示：** 容器 **不会**  magically 隔离 L3 cache — 与 VM 一样要 **物理隔离或 dedicated 核**。

### cgroups 资源控制

**CPU（cgroup v2 示例）：**

| 控制 | 文件/概念 | 效果 |
|------|-----------|------|
| **weight (shares)** | `cpu.weight` | 相对权重 |
| **bandwidth** | `cpu.max` | **硬 cap** — 如 `max 200000 100000` = 2 CPU |
| **throttle** | `cpu.stat` → **`nr_throttled` / `throttled_usec`** | 触顶证据 |

**内存 / blkio：**

- `memory.max`、`memory.high` — OOM/throttle
- `io.max` — 磁盘 IOPS/带宽限（v2）

```bash
# 容器是否被 CPU 节流（cgroup v2）
cat /sys/fs/cgroup/cpu.stat
# nr_periods nr_throttled throttled_usec ...
```

### 观测陷阱（Gregg 重点）

**容器内运行 `top` / `iostat` / `uptime` / `mpstat`：**

| 工具显示 | 实际可能是 |
|----------|------------|
| 8 CPU 全 busy | **宿主机 8 核**，容器可能只 **quota 2 核** |
| Load average 很高 | **宿主机 load**，非容器 cgroup load |
| `%iowait` | 宿主机级 |

**正确做法：**

| 层级 | 看什么 |
|------|--------|
| **容器内** | cgroup：`cpu.stat`、`memory.current`、`memory.events` |
| **宿主机** | `systemd-cgtop`、BPF 按 cgroup 过滤、`kubectl top`（API 层） |
| **K8s** | limits/requests、**CPU throttling** 指标（cAdvisor/Prometheus） |

```bash
# cgroup v2 CPU 节流
grep throttled /sys/fs/cgroup/cpu.stat

# 宿主机上 BPF 按 cgroup 追踪（需权限）
# bpftrace -e '... @cgroup = cgroup...'
```

→ Ch 4 [观测工具](../../chapter-04-observability-tools/) · Ch 15 [BPF](../../chapter-15-bpf/)

**HFT：** 即使在 **裸机** 上用 systemd/cgroup 隔离进程 — **同样要看 cgroup stat**，勿只信 `top`。

---


---

← [本章导读](../README.md)
