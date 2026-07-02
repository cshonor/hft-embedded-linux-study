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

→ 自制 OS 对照：[01 Day 5 GDT/IDT](../../../../07-system-low-level-hands-on/02-30days-os/day-05-gdt-idt/) · [Day 20 INT 0x40 API](../../../../07-system-low-level-hands-on/02-30days-os/day-20-api/)  
→ [03 SysPerf Ch3 术语](../../../../13-Systems-Performance-2nd/chapter-03-operating-systems/notes/section-3.1-核心术语.md)

---
