# 2. 容器技术基础

### 实现方式

| 类型 | 机制 | 本章重点 |
|------|------|----------|
| **OS 级容器** | **namespaces**（视图隔离）+ **cgroups**（限制） | **Docker/containerd/K8s** |
| **轻量 VM** | Kata、Firecracker 等 | 更接近 [Ch 16 虚拟化](../../chapter-16-hypervisors/) |

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
| **memory limit** | OOM kill / reclaim — [Ch 7](../../chapter-07-memory/) |
| **blkio throttle** | 未打满磁盘却 **I/O 变慢** |

**软限制：** 硬件 **未饱和** 时应用已慢 — 传统 **host 级 iostat** 可能 **一切正常**。

→ [05-LKD cgroups](../05-Linux-Kernel-Development/) · [08-TLPI cgroups](../08-The-Linux-Programming-Interface/)

---
