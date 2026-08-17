# 第5章 操作系统内核极致调优（索引）

> **原书第 6 章 §1 · 最小化上下文切换** + Linux 落地

← 原理：[chapter-04 硬件到 OS](../chapter-04-硬件选型与服务器配置/README.md) · 无锁/内存池：[chapter-07](../chapter-07-无锁数据结构与内存布局/README.md)

---

## 本章定位

原书 **Ch6 HFT Optimization（架构与 OS）** 的第一支柱：**消灭上下文切换** — 本章给出 **代价模型 + Linux 实操**；第二、三支柱（无锁、内存池）→ [chapter-07](../chapter-07-无锁数据结构与内存布局/README.md)。

## 小节索引

| 节 | 主题 | 一句话 |
|----|------|--------|
| [5.1](./5.1-上下文切换的代价.md) | 上下文切换的代价 🔴 | PCB + Cache 失效 + TLB 刷新三重税 |
| [5.2](./5.2-CPU隔离与核心绑定.md) | CPU 隔离与核心绑定 | isolcpus · nohz_full · IRQ affinity |
| [5.3](./5.3-KernelBypass.md) | Kernel Bypass 🔴 | 用户态 poll + 零拷贝，μs 级提升 |
| [5.4](./5.4-BIOS与电源.md) | BIOS / 电源 | 关 HT / C-states / Turbo |
| [5.5](./5.5-HugePages与TLB.md) | Huge Pages 与 TLB | 大页减少 TLB miss |

## 本章小结

| 目标 | 手段 |
|------|------|
| **少切换** | Pinning · isolcpus · 无阻塞 I/O |
| **少 Jitter** | 关 HT/Turbo/C-states |
| **少 syscall 路径** | Kernel Bypass |
| **少 TLB miss** | Huge Pages |

**下一支柱：** [chapter-07 无锁 + 内存池](../chapter-07-无锁数据结构与内存布局/README.md)
