# Ch 15 容器 · Containers

> **BPF Performance Tools** · Brendan Gregg · **跳过 ⚪**

> 本章定位：**Docker/K8s 下的 BPF 观测** — 底层仍是 CPU/内存/磁盘/网（Ch 6–10 工具 **大多仍适用**），但 **cgroups 软限制** 与 **namespace 隔离** 带来新坑：**吵闹邻居**、节流、宿主机跑工具、容器 ID 过滤。  
> **HFT：** 生产 **tick 路径多为裸金属/专用 VM** — 本章 **⚪ 默认跳过**；若 **风控/网关/监控** 跑在 K8s，incident 时用 **`runqlat --pidnss`、`blkthrot`、`pidnss`**。  
> **上一章：** [chapter-14-内核.md](./chapter-14-内核.md) · **下一章：** [chapter-16-虚拟机管理程序.md](./chapter-16-虚拟机管理程序.md)

---

## 1. 容器没改变什么 / 改变了什么

| 未变 | 新增 |
|------|------|
| CPU、内存、块 I/O、网络栈 | **cgroups 限额/份额/节流** |
| 前文 BPF 工具原理 | **namespace 视图** — 容器内 `top`/`free` **误导** |
| 内核路径 | **OverlayFS** 层 |

```
同一宿主机
  ├─ 容器 A（策略？）  ── cgroup CPU quota / blkio throttle
  ├─ 容器 B（日志）    ── noisy neighbor
  └─ 宿主机上跑 BPF    ── 需 pidns/uts 过滤
```

**HFT 原则：** **低延迟策略不进共享 cgroup 容器**；若必须共宿，本章工具证 **是否被 throttle**。

---

## 2. 容器技术基础

### 实现方式

| 类型 | 机制 | 本章重点 |
|------|------|----------|
| **OS 级容器** | **namespaces**（视图隔离）+ **cgroups**（限制） | **Docker/containerd/K8s** |
| **轻量 VM** | Kata、Firecracker 等 | 更接近 [Ch 16 虚拟化](./chapter-16-虚拟机管理程序.md) |

### Namespaces（与 BPF 相关）

| Namespace | 隔离 |
|-----------|------|
| **PID** | 进程号空间 — **PIDNS ID 可区分容器** |
| **UTS** | hostname — Docker/K8s 常设为 **容器 ID 片段** |
| **NET** | 网络栈 |
| **MNT** | 挂载 |
| **IPC / USER** | … |

### cgroups 与「吵闹的邻居」

| 限制类型 | 现象 |
|----------|------|
| **CPU shares / quota** | 未打满物理核却 **run queue 变长** |
| **memory limit** | OOM kill / reclaim — [Ch 7](./chapter-07-内存.md) |
| **blkio throttle** | 未打满磁盘却 **I/O 变慢** |

**软限制：** 硬件 **未饱和** 时应用已慢 — 传统 **host 级 iostat** 可能 **一切正常**。

→ [05-LKD cgroups](../05-Linux-Kernel-Development/) · [07-TLPI cgroups](../07-The-Linux-Programming-Interface/)

---

## 3. 容器环境下 BPF 的挑战

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

## 4. 传统容器分析工具

### 宿主机视角

| 工具 | 作用 |
|------|------|
| **`systemd-cgtop`** | cgroup 资源 top |
| **`kubectl top pod/node`** | K8s 聚合 |
| **`docker stats`** | 单容器 CPU/内存/IO |
| **`/sys/fs/cgroup/`** | 原始计数 |

```bash
systemd-cgtop
cat /sys/fs/cgroup/memory/kubepods.slice/.../memory.current
```

### 容器内视角 — 易误导 ⚠️

| 命令 | 陷阱 |
|------|------|
| **`top` / `htop`** | 可能显示 **宿主机 CPU 数** |
| **`free`** | 显示 **宿主机总内存**，非 **cgroup limit** |
| **`iostat`** | 可见 host 磁盘，非 **容器 blkio 视图** |

**结论：** 性能分析 **优先 host 工具 + cgroup 文件**；容器内传统命令 **仅作粗参考**。

---

## 5. 核心 BPF 容器工具

### `runqlat --pidnss`

[Ch 6 `runqlat`](./chapter-06-CPU.md) + **按 PID namespace 分桶**。

```bash
sudo runqlat-bpfcc --pidnss 10
```

| 解读 | 含义 |
|------|------|
| 某 pidns 右尾长 | 该 **容器** CPU 调度延迟高 — **quota/shares** 或争抢 |
| 裸金属对比 | 无 `--pidnss` 全局 vs 分容器 |

**HFT：** 共宿 K8s 上某 pod P99 升 → 是否 **CPU throttle**（配合 `kubectl describe` limits）。

### `pidnss`

统计调度器在 **不同 PID namespace 间切换** 的次数。

```bash
sudo pidnss-bpfcc 5
```

**回答：** 多容器是否在 **同一 CPU 上激烈交错** — noisy neighbor **直接证据**。

### `blkthrot`

统计 **cgroup 块 I/O 节流 (throttle)** 次数。

```bash
sudo blkthrot-bpfcc
```

| 解读 | 含义 |
|------|------|
| 计数增长 | 容器触碰 **blkio 速率上限** — 盘未满也慢 |

→ 衔接 [Ch 9 `biolatency`](./chapter-09-磁盘IO.md) — 块层慢但 **iostat 不忙**。

### `overlayfs`

追踪 **OverlayFS** 读写 **延迟** — 容器镜像层/可写层。

```bash
sudo overlayfs-bpfcc 10
```

**场景：** 容器内 **大量小文件读**、日志写 overlay — 比 host ext4 多一层开销。

---

## 6. 与前文章节的组合

| 问题 | 工具链 |
|------|--------|
| 容器 CPU 慢 | `runqlat --pidnss` + `pidnss` + host `runqlat` |
| 容器 I/O 慢 | **`blkthrot`** + `biolatency` + `fileslower`（Ch 8） |
| 容器网络 | Ch 10 工具在 **host** 跑 + **netns** 过滤（进阶） |
| 内存 OOM | Ch 7 `oomkill` + cgroup `memory.events` |
| 应用栈 | Ch 13 — **仍须在 host** 对容器 PID trace |

---

## 7. HFT 部署建议（与本章关系）

| 实践 | 原因 |
|------|------|
| **策略裸金属 / 专用 VM** | 避免 cgroup 软限制与 noisy neighbor |
| 辅助服务容器化 | 可接受 — 用本章工具 **隔离诊断** |
| BPF 在 **host** | 权限 + 全栈可见 |
| 不用容器内 `free` 判断内存 | 误读 limit |

**若已在 K8s 跑交易相关组件：** 检查 **CPU set / dedicated node pool / Guaranteed QoS** — 本章工具验证 **是否仍被 throttle**。

---

## 8. HFT 读者 Takeaway

1. **⚪ 默认跳过** — 裸金属 HFT 主路径；容器化 **非 tick 服务** 才需本章。
2. **Ch 6–10 工具仍有效** — 但要在 **宿主机** 跑，并加 **pidns/cgroup 过滤**。
3. **`runqlat --pidnss` + `blkthrot`** — 证 **cgroup 节流** 而非硬件瓶颈。
4. **`pidnss`** — 多容器 **CPU 交错** 竞争。
5. **容器内 top/free 误导** — 用 `docker stats`/cgroup BPF。
6. **`overlayfs`** — 容器文件 I/O 慢于 host 的 **一层原因**。
7. 更底层隔离 → [Ch 16 Hypervisors](./chapter-16-虚拟机管理程序.md)。

---

## 相关章节

- 上一章：[chapter-14-内核.md](./chapter-14-内核.md)
- 下一章：[chapter-16-虚拟机管理程序.md](./chapter-16-虚拟机管理程序.md)
- CPU 调度：[chapter-06-CPU.md](./chapter-06-CPU.md)
- 磁盘：[chapter-09-磁盘IO.md](./chapter-09-磁盘IO.md)
- SysPerf 容器/cloud：[chapter-11-cloud-computing](../02-Systems-Performance-2nd/chapter-11-cloud-computing/)（若存在）
