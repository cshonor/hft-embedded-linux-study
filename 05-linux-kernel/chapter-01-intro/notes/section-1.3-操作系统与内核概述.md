## ③ 操作系统与内核概述

#### 内核（Kernel）

**操作系统最内层** — 提供基本服务、**管理硬件**、**分配资源**。

#### 两种地址空间

```
┌─────────────────────────────────────┐
│  user-space   应用程序（策略、网关）   │
│       │ syscall ↑                     │
├───────┼─────────────────────────────┤
│  kernel-space  内核（机制）           │
│       │ ↑ 中断                        │
└───────┼─────────────────────────────┘
        硬件
```

| 空间 | 谁跑 | 权限 |
|------|------|------|
| **kernel-space** | 内核代码 | **完全硬件访问** · 受保护内存 |
| **user-space** | 普通 app | **受限** — 经 **系统调用** 求内核办事 |

| 方向 | 机制 |
|------|------|
| **app → 内核** | **System Calls**（`read`、`write`、`clone`…） |
| **硬件 → 内核** | **Interrupts** → **中断处理程序** |

→ 自制 OS 对照：[01 Day 5 GDT/IDT](../../../../projects/P9-os-from-scratch/thirty-days-os/day-05-gdt-idt/) · [Day 20 INT 0x40 API](../../../../projects/P9-os-from-scratch/thirty-days-os/day-20-api/)  
→ [03 SysPerf Ch3 术语](../../../../15-systems-performance/chapter-03-operating-systems/notes/section-3.1-核心术语.md)



<details>
<summary>自测题（点击展开）</summary>

**Q1.** 内核态和用户态切换的代价是什么？HFT 如何减少这种切换？

<details><summary>答案</summary>

一次 syscall 需要：保存用户态寄存器 → 切换栈 → 进入内核态执行 → 恢复寄存器返回。x86_64 约 100-200ns。HFT 减少 syscall 的方法：批量 IO（io_uring）、内存映射（mmap 替代 read）、绑定 CPU 避免调度、busy-polling 替代中断等待。

</details>

**Q2.** 内核有哪些基本职责？为什么 HFT 工程师必须理解它们？

<details><summary>答案</summary>

内核管理 CPU 调度、内存分配、文件系统、网络协议栈、设备驱动、安全隔离。HFT 的每纳秒延迟都与这些子系统相关：调度器决定你的线程何时运行、内存分配器决定 TLB miss 率、网络栈决定包处理路径。不理解内核就无法做极限优化。

</details>

</details>
---
