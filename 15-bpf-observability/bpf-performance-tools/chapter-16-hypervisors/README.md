# Ch 16 虚拟机管理器 · Hypervisors

> **BPF Performance Tools** · Brendan Gregg · 第 16 章（印刷 p719–737）

> 本章定位：**硬件虚拟化（Xen/KVM）上的 BPF** — [Ch 15 容器](../chapter-15-containers/) 是 OS 级隔离；本章是 **Guest VM ↔ Hypervisor** 边界。需分别从 **访客机 (Guest)** 与 **宿主机 (Host)** 两侧观测。
> **HFT：** 生产 **tick 路径优先裸金属**；若跑在 **云 VM / 托管 KVM** 上，Guest 侧 **`cpustolen`** 与 Host 侧 **`kvmexits`** 可证 **底层争抢**；**AWS Nitro** 等架构需退回 Ch 6–10 **通用资源工具**。
> **上一章：** [chapter-15-容器](../chapter-15-containers/) · **下一章：** [chapter-17-其他BPF工具](../chapter-17-other-tools/)

---

## 原书真实小节 → 笔记映射

| 原书小节 | 笔记 |
|----|------|
| 16.1 背景知识（BPF 分析能力 / 建议的分析策略） | [notes/section-1-背景知识.md](./notes/section-1-背景知识.md) |
| 16.2 传统工具（xl top / xentrace / perf kvm stat） | [notes/section-2-传统工具.md](./notes/section-2-传统工具.md) |
| 16.3.1 Xen 超级调用（funccount / argdist / stackcount / funclatency） | [notes/section-3-BPF工具-Xen超级调用.md](./notes/section-3-BPF工具-Xen超级调用.md) |
| 16.3.2 xenhyper / 16.3.3 Xen 回调 | [notes/section-4-BPF工具-xenhyper与Xen回调.md](./notes/section-4-BPF工具-xenhyper与Xen回调.md) |
| 16.3.4 cpustolen / 16.3.5 HVM 退出跟踪（hyperupcalls） | [notes/section-5-BPF工具-cpustolen与HVM退出跟踪.md](./notes/section-5-BPF工具-cpustolen与HVM退出跟踪.md) |
| 16.4 宿主机 BPF 工具（16.4.1 kvmexits / 16.4.2 未来的工作） | [notes/section-6-BPF工具-宿主机kvmexits与未来工作.md](./notes/section-6-BPF工具-宿主机kvmexits与未来工作.md) |
| 16.5 小结 | [notes/section-7-小结.md](./notes/section-7-小结.md) |
| （延伸）Part II（第 11–16 章）收官回顾 | [notes/section-8-PartII总结与工具全景.md](./notes/section-8-PartII总结与工具全景.md) |

---

## 本章要点

- **两种配置**：裸机管理器（Xen，零号域管理）vs 宿主机内核模块（KVM + QEMU 代理）
- **可观测性随硬件化收缩**：PV（超级调用+回调+盗用）→ PVHVM → +SR-IOV → Nitro（仅盗用时间）
- **访客侧工具**：xenhyper（按名称计超级调用）、cpustolen（kretprobe stealclock 差值直方图）、xen_evtchn_do_upcall 回调延迟
- **宿主侧工具**：kvmexits（kvm_exit/kvm_entry 跟踪点对 + vmx.h 手写退出原因表）；无内核 KVM 模块时退回 uprobes 插桩 qemu
- **未来方向**：guest_rip 采样受困于符号表/地址空间鸿沟，hyperupcalls 尝试用 BPF 跨越边界

---

## 本章 Checklist

- [ ] **`cpustolen` / `%st`**— 「代码没问题但 P99 抖」的第一层 **基础设施证伪**。
- [ ] **`kvmexits`（Host）**— 运维/平台团队查 **oversubscription、异常 exit**。
- [ ] **Nitro 类架构**— 少依赖 Xen 专用工具；**通用 BPF + 云厂商指标**。
- [ ] **Guest 内仍可跑 Ch 3 清单**— 但解读时记得 **结果含虚拟化 tax**。
- [ ] **深度栈分析在 Guest**— Host 只有 exit 原因与 RIP，无 Guest 符号。

---

## 相关章节

- 上一章：[chapter-15-容器](../chapter-15-containers/)
- 下一章：[chapter-17-其他BPF工具](../chapter-17-other-tools/)
- CPU stolen / runqlat：[chapter-06-cpus](../chapter-06-cpus/)
- 云/虚拟化：[14-systems-performance](../../../14-systems-performance/)
- Hennessy 虚拟化：[17-computer-architecture](../../../17-computer-architecture/)
