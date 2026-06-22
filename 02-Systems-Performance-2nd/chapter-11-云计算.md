# Ch 11 云计算 · Cloud Computing

> **Systems Performance 2nd** · Brendan Gregg · **跳过**（HFT 共置裸机主路径 ⚪；云/K8s 部署时 🟡 按需）

> 本章定位：**云解决了扩展与部署，但引入虚拟化开销与多租户争用** — 「吵闹的邻居」、容器内 `top` 骗人、VM-EXIT、SR-IOV/Nitro 等。Gregg 第二版副标题即 *Enterprise and the Cloud*，本章是 **云原生环境的性能地图**。  
> **HFT：** 低延迟共置/托管 **以裸机为主**，本章 **非主线**；若研究环境、风控、监控、回测上云，或混部 K8s — 读 **11.3 容器观测陷阱 + cgroups** 即可避坑。

---

## 大白话 · 本章就五件事

> **云上性能 = 真实负载 + 虚拟化税 + 邻居噪声。**

**① 水平扩展 vs 垂直扩展 — 容量模型变了。**

- 云靠 **多实例 + 负载均衡 + 分片 DB**；按需计费、自动伸缩 — 也要防 **overprovisioning 烧钱**。

**② 硬件虚拟化（VM）：Hypervisor + VM-EXIT + I/O 代理。**

- KVM/Xen/VMware — 客户机只见虚拟资源；**SR-IOV / Nitro** 直通网卡接近裸机。
- 排障：**宿主机** 才能看全局；Guest 内 perf 遇物理瓶颈可能 **看不全**。

**③ 容器（OS 虚拟化）：Namespaces + cgroups，几乎无额外 CPU 映射开销。**

- 最大问题：**多租户争用** cache/TLB/锁/网络。
- **陷阱：** 容器里 `top`/`iostat`/`uptime` 常显示 **整台宿主机** — 要看 **`cpu.stat` throttled_time** 等 cgroup 指标。

**④ 轻量虚拟化：Firecracker 等 MicroVM — 隔离 + 快启。**

- 内存开销可 < 5MB；观测类似 KVM，Guest 内可用 BPF。

**⑤ FaaS / Unikernels — 冷启动与观测性代价。**

- Serverless **冷启动**；传统 perf 工具受限 — HFT 热路径 **不适用**。

下面按原书 11.1–11.5 展开。

---

## 11.1 云计算背景与架构

### 水平扩展（Horizontal Scalability）

| 模式 | 做法 | 典型组件 |
|------|------|----------|
| **垂直扩展** | 单机更大 CPU/内存 | 传统大型机、裸机 scale-up |
| **水平扩展** | 更多实例分担负载 | LB + 无状态 app 集群 + 分片存储 |

```
                    Load Balancer
                   /      |      \
              Web/App   Web/App   Web/App
                   \      |      /
                  Sharded / Cloud-native DB
```

**HFT 对比：**

- **tick 热路径**：垂直扩展 + **绑核裸机** — 不靠水平复制同一策略实例（状态难拆）。
- **可水平部分**：行情 fan-out、回测 worker、监控、研究 notebook — 适合云。

### 容量规划与动态缩放

| 机制 | 好处 | 风险 |
|------|------|------|
| **按需计费** | 用多少付多少 | 忘记关机 → 账单 |
| **Auto Scaling** | 负载涨 → 加实例 | 缩放滞后、冷实例 |
| **Bursting** | 短时超配 CPU credits | credits 耗尽 → **性能 cliff** |

**监控：** 不仅看 CPU%，还要看 **P99 延迟、throttle、成本/请求** — 防 overprovisioning。

### 多租户与编排（Kubernetes）

| 概念 | 性能影响 |
|------|----------|
| **Multi-tenancy** | 共享物理机 → **noisy neighbor** |
| **Kubernetes Pod** | 调度、CNI 网络、overlay 增 hop |
| **CNI** | VXLAN/iptables/eBPF — 吞吐与延迟特征不同 |

**HFT：** 生产策略 **避免** 与未知负载同节点；若 K8s 跑 **非热路径**（/grafana/批任务），需 **Dedicated node pool + tolerations**。

→ Ch 10 [网络](./chapter-10-网络.md) · Ch 6/7 [cgroups](./chapter-06-中央处理器.md#69-cpu-调优)

---

## 11.2 硬件虚拟化（Hardware Virtualization）

### Hypervisor 与常见实现

| 类型 | 例子 | 特点 |
|------|------|------|
| **Type 1** | VMware ESXi、Hyper-V、Xen（部分） | 裸金属 Hypervisor |
| **Type 2 / 模块** | **KVM**（Linux 内核模块） | 宿主机也是 Linux |

每个 **VM** = 完整 Guest OS + 虚拟 vCPU/vNIC/vDisk。

### 性能开销来源

| 开销 | 说明 |
|------|------|
| **VM-EXIT / VM-ENTRY** | Guest ↔ Hypervisor 上下文切换 |
| **I/O 代理** | 虚拟磁盘/网卡经 Hypervisor 软件路径 — **延迟高** |
| **嵌套页表** | 内存虚拟化 — TLB 压力 |
| **Steal time** | Guest 内可见 — **vCPU 被宿主机抢走** 的时间 |

**Guest 内 `top`：** `%st`（steal）高 → **物理 CPU 争用**，非 Guest 算力足。

### 硬件直通与 Nitro

| 技术 | 效果 |
|------|------|
| **PCI Passthrough / SR-IOV** | 网卡/ NVMe **直通** Guest — 绕过软件 I/O 代理 |
| **AWS Nitro** | 网络/存储 offload 到专用硬件 — **接近裸机** 网络 |

**HFT：** 若必须在云上跑延迟敏感组件 — 选 **裸金属实例 / SR-IOV / 增强网络**，勿用普通虚拟网卡 + 共享 tenancy。

### 资源控制与观测

| 控制 | 机制 |
|------|------|
| vCPU 数量 | 限算力 |
| **Balloon driver** | 从 Guest **回收内存** 给宿主机 — Guest 内可用内存 **突然降** |
| 内存 overcommit | 宿主机卖超 — 触发 swap/压缩 |

| 观测位置 | 工具 |
|----------|------|
| **宿主机** | `kvm_stat`、`perf kvm`、`xentop`、Hypervisor CLI |
| **Guest 内** | 常规 perf/BPF — **见虚拟资源**，物理瓶颈可能 **不透明** |

**排障原则：** 延迟尖刺在 Guest 内无法解释 → **升 ticket 看宿主机** 或迁 **dedicated host**。

---

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

→ Ch 4 [观测工具](./chapter-04-观测工具.md) · Ch 15 [BPF](./chapter-15-BPF技术.md)

**HFT：** 即使在 **裸机** 上用 systemd/cgroup 隔离进程 — **同样要看 cgroup stat**，勿只信 `top`。

---

## 11.4 轻量级硬件虚拟化（MicroVM）

### Firecracker 等

| 特点 | 说明 |
|------|------|
| **精简 Hypervisor** | 代码面小 — AWS Lambda/Fargate 等底层 |
| **无传统 BIOS/PCI 模拟** | 启动快、内存 **< 5MB** 级开销 |
| **隔离性** | 强于容器（独立 Guest 内核） |
| **密度** | 高于传统 VM |

**观测：** 与 KVM 类似 — **Guest 内有完整内核**，可用 **perf/BPF**；宿主机仍用 Hypervisor 工具。

**HFT：** 研究/沙箱隔离可用；**共置 tick 引擎** 仍优先 **裸金属** 而非 MicroVM。

---

## 11.5 其他云技术

### FaaS（Serverless）

| 优点 | 性能代价 |
|------|----------|
| 免运维、按调用计费 | **冷启动**（容器/MicroVM 拉起 + runtime init） |
| 自动扩展 | **无法** 传统 SSH + 全套 perf 栈 — 观测靠 **平台日志/Tracing** |

**HFT：** 适合 **偶发任务**（报表、告警）；**绝不** 放 tick/发单热路径。

### Unikernels

| 特点 | 代价 |
|------|------|
| 应用 + 极简 libOS **单一镜像** | 极致性能潜力 |
| 无完整通用 OS | **观测工具缺乏**、调试难 |

**HFT：** 学术/极端场景；工程主流仍是 **Linux 裸机 + DPDK**。

---

## 本章 Checklist

- [ ] 能区分 **水平 vs 垂直扩展** 与 HFT 各自适用场景
- [ ] 知道 **VM-EXIT、steal time、SR-IOV/Nitro** 是什么
- [ ] 理解 **容器内 top/iostat 误导** — 会查 **`cpu.stat` throttled**
- [ ] 知道 **Namespaces vs cgroups** 分工
- [ ] 云实例排障时知道 **Guest 不够 → 宿主机视角**
- [ ] 明确 **FaaS/Unikernels 非 HFT 热路径技术**

---

## HFT 精读捷径（Ch 11 在路线中的位置）

```
共置/托管裸机 HFT 主路径：
  Ch 1–10 → Ch 13/15 → 07–10 网络栈 → 11-HFT
  Ch 11 云计算 → ⚪ 整章可跳过

若涉及云/K8s/混合部署：
  精读 11.1（多租户）+ 11.3（cgroups + 观测陷阱）
  粗读 11.2（VM steal、SR-IOV 选型）
  11.4–11.5 了解即可
```

**按需最小行动集（容器环境）：**

1. 读 Pod/容器的 **CPU limit** 与 **`cpu.stat` throttled_usec**。
2. 对比 **容器内 mpstat** vs **宿主机 cgroup 统计** — 确认是否看错层。
3. 延迟敏感 workload → **Dedicated node / 裸金属 / SR-IOV**。

**Gregg 本章金句（HFT 版）：**

> 云 **扩展了部署，也扩展了不确定性** — noisy neighbor 和 **容器内假象** 是两大坑。  
> HFT 要低延迟：**先选裸机与共置**；上云的是 **研究与非热路径**，且 **永远查 cgroup，别信容器里的 top**。

---

## 相关章节

- 上一章：[chapter-10-网络.md](./chapter-10-网络.md)
- 下一章：[chapter-12-基准测试.md](./chapter-12-基准测试.md)
- cgroups / CPU：[chapter-06-中央处理器.md](./chapter-06-中央处理器.md)
- 内存 cgroup：[chapter-07-内存.md](./chapter-07-内存.md)
- 磁盘 cgroup：[chapter-09-磁盘.md](./chapter-09-磁盘.md)
- OS 模型：[chapter-03-操作系统.md](./chapter-03-操作系统.md)
- BPF 观测：[chapter-15-BPF技术.md](./chapter-15-BPF技术.md)
- HFT 裸机调优：[11-HFT ch05](../11-HFT-Low-Latency-Practice/chapter-05-操作系统内核极致调优.md)
- 全书目录：[OUTLINE.md](./OUTLINE.md)
