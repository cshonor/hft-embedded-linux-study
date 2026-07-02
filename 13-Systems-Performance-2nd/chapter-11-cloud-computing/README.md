# Ch 11 云计算 · Cloud Computing

> **Systems Performance 2nd** · Brendan Gregg · **跳过**（HFT 共置裸机主路径 ⚪；云/K8s 部署时 🟡 按需）

> 本章定位：**云解决了扩展与部署，但引入虚拟化开销与多租户争用** — 「吵闹的邻居」、容器内 `top` 骗人、VM-EXIT、SR-IOV/Nitro 等。Gregg 第二版副标题即 *Enterprise and the Cloud*，本章是 **云原生环境的性能地图**。  
> **HFT：** 低延迟共置/托管 **以裸机为主**，本章 **非主线**；若研究环境、风控、监控、回测上云，或混部 K8s — 读 **11.3 容器观测陷阱 + cgroups** 即可避坑。

---

## 小节笔记

| 节 | 笔记 |
|----|------|
| 11.1 云计算背景与架构 | [notes/section-11.1-云计算背景与架构.md](./notes/section-11.1-云计算背景与架构.md) |
| 11.2 硬件虚拟化（Hardware Virtualization） | [notes/section-11.2-硬件虚拟化Hardware-Virtualization.md](./notes/section-11.2-硬件虚拟化Hardware-Virtualization.md) |
| 11.3 操作系统虚拟化 / 容器 | [notes/section-11.3-操作系统虚拟化-容器.md](./notes/section-11.3-操作系统虚拟化-容器.md) |
| 11.4 轻量级硬件虚拟化（MicroVM） | [notes/section-11.4-轻量级硬件虚拟化MicroVM.md](./notes/section-11.4-轻量级硬件虚拟化MicroVM.md) |
| 11.5 其他云技术 | [notes/section-11.5-其他云技术.md](./notes/section-11.5-其他云技术.md) |

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
  Ch 1–10 → Ch 13/15 → 08–11 网络栈 → 12-HFT
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

- 上一章：[../chapter-10-network/](../chapter-10-network/)
- 下一章：[../chapter-12-benchmarking/](../chapter-12-benchmarking/)
- cgroups / CPU：[../chapter-06-cpus/](../chapter-06-cpus/)
- 内存 cgroup：[../chapter-07-memory/](../chapter-07-memory/)
- 磁盘 cgroup：[../chapter-09-disks/](../chapter-09-disks/)
- OS 模型：[../chapter-03-operating-systems/](../chapter-03-operating-systems/)
- BPF 观测：[../chapter-15-bpf/](../chapter-15-bpf/)
- HFT 裸机调优：[12-HFT ch05](../../15-HFT-Low-Latency-Practice/chapter-05-操作系统内核极致调优/)
- 全书目录：[OUTLINE.md](../OUTLINE.md)
