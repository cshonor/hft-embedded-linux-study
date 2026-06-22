## 3.1 核心术语

| 术语 | 含义 | 性能分析中的问法 |
|------|------|------------------|
| **OS** | 操作系统 | 版本、补丁、发行版差异 |
| **Kernel** | 内核 | 调度、内存、网络栈在哪一层 |
| **Process** | 进程 | 哪个 PID、多少内存、多少 fd |
| **Thread** | 线程 | 哪个 TID 吃 CPU、是否绑核 |
| **Context switch** | 上下文切换 | `pidstat -w`、run queue、cache 冷 |
| **Mode switch** | 模式切换 | 用户↔内核，syscall 路径 |
| **System call** | 系统调用 | `read`/`write`/`send`/`mmap`/`clone`… |
| **Hardware interrupt** | 硬件中断 | IRQ、软中断、网卡收包路径 |

**HFT：** 延迟分解里若出现「内核段」不明 — 先查是 **syscall**、**缺页**、**中断/softirq** 还是 **调度切换**。

→ 用户/内核分界与 syscall 流程：[02 内核架构 a03](../../../05-Linux-Kernel-Development/03_Course_Kernel_Architecture/episode-a03-内核架构总览.md)

---


---

← [本章导读](../README.md)
