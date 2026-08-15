# 4. 传统容器分析工具

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
