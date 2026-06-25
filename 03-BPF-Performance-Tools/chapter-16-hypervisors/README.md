# Ch 16 虚拟机管理程序 · Hypervisors

> **BPF Performance Tools** · Brendan Gregg · **跳过 ⚪**

> 本章定位：**硬件虚拟化（Xen/KVM）上的 BPF** — [Ch 15 容器](../chapter-15-containers/) 是 OS 级隔离；本章是 **Guest VM ↔ Hypervisor** 边界。需分别从 **访客机 (Guest)** 与 **宿主机 (Host)** 两侧观测。  
> **HFT：** 生产 **tick 路径优先裸金属**；若跑在 **云 VM / 托管 KVM** 上，Guest 侧 **`cpustolen`** 与 Host 侧 **`kvmexits`** 可证 **底层争抢**；**AWS Nitro** 等架构需退回 Ch 6–10 **通用资源工具**。  
> **上一章：** [chapter-15-容器.md](../chapter-15-containers/) · **下一章：** [chapter-17-其他BPF工具.md](../chapter-17-other-tools/)

---

## 小节笔记

| 节 | 笔记 |
|----|------|
| 1 Part II 收官：虚拟化在栈中的位置 | [notes/section-1-PartII收官虚拟化在栈中的位置.md](./notes/section-1-PartII收官虚拟化在栈中的位置.md) |
| 2 硬件虚拟化基础 | [notes/section-2-硬件虚拟化基础.md](./notes/section-2-硬件虚拟化基础.md) |
| 3 分析策略：Guest vs Host | [notes/section-3-分析策略GuestvsHost.md](./notes/section-3-分析策略GuestvsHost.md) |
| 4 传统分析工具 | [notes/section-4-传统分析工具.md](./notes/section-4-传统分析工具.md) |
| 5 Guest BPF 工具 | [notes/section-5-GuestBPF工具.md](./notes/section-5-GuestBPF工具.md) |
| 6 Host BPF 工具 | [notes/section-6-HostBPF工具.md](./notes/section-6-HostBPF工具.md) |
| 7 工具选型速查 | [notes/section-7-工具选型速查.md](./notes/section-7-工具选型速查.md) |
| 8 Part II 总结 | [notes/section-8-PartII总结.md](./notes/section-8-PartII总结.md) |

---

## 大白话

> 硬件虚拟化（Xen/KVM）上的 BPF

下面按原书小节展开；细节见 **小节笔记** 表。

---

## 本章 Checklist

- [ ] **⚪ 默认跳过**— 裸金属为主；**云 VM / 托管 KVM** 时本章 **选读**。
- [ ] **`cpustolen` / `%st`**— 「代码没问题但 P99 抖」的第一层 **基础设施证伪**。
- [ ] **`kvmexits`（Host）**— 运维/平台团队查 **oversubscription、异常 exit**。
- [ ] **Nitro 类架构**— 少依赖 Xen 专用工具；**通用 BPF + 云厂商指标**。
- [ ] **Guest 内仍可跑 Ch 3 清单**— 但解读时记得 **结果含虚拟化 tax**。
- [ ] **深度栈分析在 Guest**— Host 只有 exit 原因与 RIP，无 Guest 符号。

---

## 相关章节

- 上一章：[chapter-15-容器.md](../chapter-15-containers/)
- 下一章：[chapter-17-其他BPF工具.md](../chapter-17-other-tools/)
- CPU stolen / runqlat：[chapter-06-CPU.md](../chapter-06-cpus/)
- 云/虚拟化：[chapter-11-cloud-computing](../../02-Systems-Performance-2nd/chapter-11-cloud-computing/)（若存在）
- Hennessy 虚拟化：[04-Computer-Architecture-6th](../04-Computer-Architecture-6th/)
