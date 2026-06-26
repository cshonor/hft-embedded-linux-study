# Ch 3 操作系统 · Operating Systems

> **Systems Performance 2nd** · Brendan Gregg · **选读**（HFT：背景速成，遇瓶颈再回查）

> 本章定位：**OS / 内核速成指南** — 性能调优时要对系统行为提假设并验证（syscall 怎么走、调度怎么分核、内存压力怎么表现、I/O 怎么缓存），不懂内核就容易猜错层。Ch 2 给了方法论；本章补**假设所依赖的底层模型**，为 Ch 5–10 的资源剖析打底。

---

## 小节笔记

| 节 | 笔记 |
|----|------|
| 3.1 核心术语 | [notes/section-3.1-核心术语.md](./notes/section-3.1-核心术语.md) |
| 3.2 内核基础与核心概念 | [notes/section-3.2-内核基础与核心概念.md](./notes/section-3.2-内核基础与核心概念.md) |
| 3.3–3.4 内核演进与 Linux 特性 | [notes/section-3.3-3.4-内核演进与-Linux-特性.md](./notes/section-3.3-3.4-内核演进与-Linux-特性.md) |
| 3.5 其他系统模型 | [notes/section-3.5-其他系统模型.md](./notes/section-3.5-其他系统模型.md) |

---

## 大白话 · 本章就四件事

> 不用背完整内核源码；知道「谁在什么态、什么路径会慢」就够排查。

**① 先分清几个词：OS、内核、进程、线程。**

- **OS** = 管硬件 + 给程序跑的环境；**内核** = OS 里始终驻留、有特权的那部分。
- **进程** = 一份独立地址空间 + 资源；**线程** = 进程里的执行流，共享地址空间。
- 量化里：行情进程、发单线程、绑核、亲和性 — 都建立在这套模型上。

**② 两种「切换」别混：模式切换 vs 上下文切换。**

| | 模式切换 | 上下文切换 |
|---|----------|------------|
| **发生什么** | 用户态 ↔ 内核态 | 换一条线程/进程跑 |
| **典型触发** | **系统调用**、缺页、部分异常 | 时间片用完、阻塞 I/O、抢占 |
| **HFT 关注** | 热路径 syscall 越少越好 | 少切换 = 少 cache 冷、少调度抖动 |

**③ 内核在帮你做五类事（知道即可，细节见后续专章）。**

```
系统调用 / 中断  →  调度（谁占哪个 CPU）
       ↓
虚拟内存 / 分页  →  I/O 与页缓存（VFS）
       ↓
SMP / cgroups   →  多核与资源限额
```

**④ Linux 现代性能三件大事：systemd 启动分析、KPTI 的 syscall 开销、eBPF 观测。**

- HFT 裸机：**KPTI / THP / 调度策略** 要心里有数；**BPF** 是 Ch 15 和整本 handbook 的观测主线。

下面按原书 3.1–3.5 展开。

---

## 内核路径速查 · HFT 延迟从哪来

```
用户策略代码
  │ syscall（mode switch）
  ├─► 缺页 / mmap 路径
  ├─► 锁竞争 → 调度（context switch）
  ├─► 内核网络栈 send/recv
  │     └─ IRQ → softirq → 协议栈
  └─► KPTI / TLB 刷新（每次进内核的隐性税）

旁路方向：mmap 大页、mlock、绑核、isolcpus、DPDK/XDP、少线程少切换
```

---

## 本章在全书中的位置

```
Ch 1–2  目标 + 方法论（USE / 延迟分解）
  → Ch 3  OS/内核模型（本章：假设与验证的「物理定律」）
  → Ch 4  观测工具
  → Ch 5  应用程序
  → Ch 6–7 CPU / 内存
  → Ch 8–9 文件系统 / 磁盘（HFT 多 ⚪）
  → Ch 10 网络
  → Ch 13–15 perf / Ftrace / BPF
```

**HFT 读法：** 本章 **通读一遍建地图**；深入某块时跳对应专章（VMM、LKD、内核网络、SysPerf Ch 6/7/10）。

---

## 本章学习目标 · 自检

- [ ] 能区分 **context switch** 与 **mode switch**，并各举 syscall / 调度例子
- [ ] 能描述 **syscall → 可能缺页 / 切换** 的链条
- [ ] 知道 **IRQ / softirq** 与网络收包的关系
- [ ] 能解释 **虚拟内存、分页、swap** 为何导致延迟尖刺
- [ ] 知道 **VFS + page cache** 在 I/O 路径中的位置
- [ ] 了解 **SMP、亲和性、cgroups** 在绑核/隔离中的含义
- [ ] 知道 **KPTI、THP、eBPF** 三者在 HFT 环境下的利弊指向

---

## HFT 精读捷径（Ch 3 最小行动集）

1. 画一条 **发单路径**：用户态 → 哪些 syscall → 是否经内核网络栈。
2. 对照 **绑核/isolcpus** 配置，标 housekeeping 核 vs hot 核。
3. 确认 **swap 关闭 / 关键内存 mlock**；THP 策略与 [note-THP](../../07-Linux-Virtual-Memory-Manager/note-透明大页THP.md) 一致。
4. 装 bpftrace，跑一条 **syscall 计数**（预告 Ch 15）验证热路径 syscall 量。

---

## 相关章节

- 上一章：[../chapter-02-methodologies/](../chapter-02-methodologies/)
- 下一章：[../chapter-04-observability-tools/](../chapter-04-observability-tools/)
- 应用程序：[../chapter-05-applications/](../chapter-05-applications/)
- CPU / 内存：[../chapter-06-cpus/](../chapter-06-cpus/) · [../chapter-07-memory/](../chapter-07-memory/)
- 网络：[../chapter-10-network/](../chapter-10-network/)
- BPF：[../chapter-15-bpf/](../chapter-15-bpf/)
- 内核架构课：[02/03_Course_Kernel_Architecture](../../05-Linux-Kernel-Development/03_Course_Kernel_Architecture/)
- 全书目录：[OUTLINE.md](../OUTLINE.md)
