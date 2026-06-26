# 2. CPU 基础知识 (Background)

### CPU 模式

| 模式 | 说明 | 传统工具中的体现 |
|------|------|------------------|
| **用户态** | 应用代码 | `top` 的 `%us` |
| **内核态** | 系统调用、驱动、协议栈 | `%sy` |
| **空闲 / iowait / steal** | 等 I/O、虚拟化偷跑等 | `%id`、`%wa`、`%st` |

**HFT：** 策略热路径应 **大部分在用户态**；`%sy` 突增 → 查 syscall 风暴或内核网络栈（衔接 [Ch 10 网络](../../chapter-10-networking/)）。

### CPU 调度器与线程状态

调度器在 **任务（线程）** 之间分配 CPU 时间片：

| 状态 | 含义 | BPF 相关 |
|------|------|----------|
| **ON-CPU** | 正在某核上运行 | `profile`、`cpudist` |
| **RUNNABLE** | 就绪，在 **运行队列** 等 CPU | `runqlat`、`runqlen`、`runqslower` |
| **SLEEP** | 阻塞（I/O、锁、futex…） | `offcputime` |

→ 内核实现对照：[05-LKD Ch 4 调度](../05-Linux-Kernel-Development/00_Book_3rd_Notes/chapter-04-process-scheduling/)

### CPU 缓存与 TLB

现代负载常为 **内存/缓存密集型**，不单看 GHz：

| 层级 | 作用 |
|------|------|
| **L1 / L2** |  per-core，最快 |
| **L3 (LLC)** | 末级缓存，多核共享 |
| **TLB** | 虚拟地址 → 物理页表项缓存 |

**工具：** `perf` PMC、`llcstat`（BPF + 硬件计数）看 LLC 命中/未命中 — 与 [CSAPP Ch6 存储层次](../01-CSAPP-3rd/chapter-06-memory-hierarchy/) 对照。

→ SysPerf CPU 章：[chapter-06-cpus](../../../03-Systems-Performance-2nd/chapter-06-cpus/)

---
