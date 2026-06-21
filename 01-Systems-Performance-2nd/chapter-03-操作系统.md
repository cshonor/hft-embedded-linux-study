# Ch 3 操作系统 · Operating Systems

> **Systems Performance 2nd** · Brendan Gregg · **选读**（HFT：背景速成，遇瓶颈再回查）

> 本章定位：**OS / 内核速成指南** — 性能调优时要对系统行为提假设并验证（syscall 怎么走、调度怎么分核、内存压力怎么表现、I/O 怎么缓存），不懂内核就容易猜错层。Ch 2 给了方法论；本章补**假设所依赖的底层模型**，为 Ch 5–10 的资源剖析打底。

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

→ 用户/内核分界与 syscall 流程：[02 内核架构 a03](../02-Linux-Kernel-Development/03_Course_Kernel_Architecture/episode-a03-内核架构总览.md)

---

## 3.2 内核基础与核心概念

### 运行模式与系统调用

处理器至少分两种特权级：

| 模式 | 谁跑 | 能做什么 |
|------|------|----------|
| **用户态（User Mode）** | 应用程序 | 不能直接碰硬件、不能改页表 |
| **内核态（Kernel Mode）** | 内核 | 特权指令、设备、内存管理 |

用户程序通过 **系统调用** 请求内核服务，例如：

| 调用 | 典型用途 | HFT 相关 |
|------|----------|----------|
| `read` / `write` | 文件/管道 I/O | 日志、配置（热路径常避免） |
| `send` / `recv` | 套接字 | 走内核协议栈时的必经路径 |
| `mmap` | 映射文件/匿名内存 | 大页、共享内存、ring buffer |
| `clone` / `fork` | 创建线程/进程 | 进程模型、线程池 |

**一次 syscall 可能涉及：**

1. **模式切换**（用户 → 内核 → 用户）
2. 若当前线程让出 CPU → **上下文切换**
3. 若触发缺页 → 额外内存管理路径

**调优直觉：** 热路径 **少 syscall、少拷贝、少阻塞**；内核旁路见 [12-DPDK](../12-DPDK-Low-Latency-Network/)、用户态栈。

---

### 中断机制（Interrupts）

| 类型 | 触发 | 例子 |
|------|------|------|
| **硬件中断（IRQ）** | 外设异步 | 网卡收包、磁盘完成 |
| **同步中断 / 陷阱** | 指令或 CPU 事件 | 系统调用入口、异常、**缺页 fault** |

Linux 为降低延迟影响，常把处理拆成：

```
上半部（Top half）  — 尽快 ACK、最小工作
下半部（Bottom half）— softirq、tasklet、ksoftirqd 稍后处理
```

**HFT：** 高频收包 → **硬中断 + NAPI + softirq** 占 CPU；`mpstat`/`perf` 里看到 `%soft` 高要往网络栈查。→ [06 内核网络 Ch 14 NAPI/RSS](../06-Linux-Kernel-Networking/)

---

### 进程与调度（Schedulers）

**进程状态（简化）：**

```
创建 → 就绪 ⇄ 运行 → 睡眠（等 I/O/锁）→ 退出 →（若父未 wait）僵尸
```

**调度器职责：** 在多 CPU 上决定**下一个跑谁**、跑多久。

| 负载类型 | 调度倾向 |
|----------|----------|
| **I/O 密集** | 阻塞时让出 CPU，唤醒后竞争时间片 |
| **CPU 密集** | 时间片轮转 / CFS 公平份额 |

**CPU 亲和性（Affinity）：** 调度器倾向把线程留在**同一 CPU**，保留 **cache warmth**（L1/L2 仍热）。

**HFT 实践：**

- **绑核（pinning）**：行情解析、策略、发单分池，避免与 OS/中断线程抢核
- **isolcpus / 专用核**： housekeeping 与 hot path 分离 → [10-HFT ch05](../10-HFT-Low-Latency-Practice/chapter-05-操作系统内核极致调优.md)
- Linux 里程碑：**O(1) 调度器** → **CFS**（完全公平调度，默认策略）

→ 深入进程/调度：[LKD 3rd Ch 3](../02-Linux-Kernel-Development/00_Book_3rd_Notes/chapter-03-进程管理.md)、Ch 4 调度

---

### 内存管理

**虚拟内存（Virtual Memory）：**

- 每个进程见**独立虚拟地址空间**
- 支持 **overcommit**（承诺超过物理内存，靠按需分配 + swap 兜底）

**分页（Paging）：**

- 物理页 ↔ 虚拟页映射；压力时 **page reclaim**、**swap**（Linux 匿名页换出常称 swapping）

| 现象 | 性能含义 |
|------|----------|
| **缺页 fault（major）** | 需读盘/映射，延迟尖刺 |
| **minor fault** | 分配/零页，相对便宜 |
| **swap 活动** | 热路径灾难性 — HFT 通常 **mlock / 预留 / 禁 swap** |

→ 精读：[03-Linux-Virtual-Memory-Manager](../03-Linux-Virtual-Memory-Manager/) Ch 3 页表/TLB/大页；[note-透明大页 THP](../03-Linux-Virtual-Memory-Manager/note-透明大页THP.md)

→ SysPerf 专章：[Ch 7 内存](./chapter-07-内存.md)

---

### I/O 与文件系统

**VFS（Virtual File System）：** 统一抽象，`ext4`/`xfs`/管道/套接字等走同一套接口。

**I/O 栈与缓存：**

```
应用 read/write
  → VFS
    → 页缓存（Page Cache）— 命中则免磁盘
      → 块层 → 磁盘
```

**性能要点：** 读写过文件系统 = 可能进 **page cache**；HFT 热路径多为 **内存 + 网络**，磁盘/FS 多为日志与配置（SysPerf Ch 8/9 可 ⚪）。

---

### 多处理器与资源控制

| 概念 | 含义 | HFT 相关 |
|------|------|----------|
| **SMP** | 对称多处理，多核共享内存 | NUMA 拓扑、跨 node 访问 |
| **IPI** | 核间中断 | TLB shootdown、调度迁移 |
| **Kernel preemption** | 内核可被打断 | 低延迟内核配置常细调抢占 |
| **cgroups** |  cgroup 限额 CPU/内存/IO | 容器环境；裸机共置时防邻居干扰 |

→ NUMA / CPU：[Ch 6](./chapter-06-中央处理器.md)；容器与云：[Ch 11](./chapter-11-云计算.md)（HFT 常 ⚪）

---

## 3.3–3.4 内核演进与 Linux 特性

### 历史演变（选读摘要）

|  lineage | 对现代性能的影响 |
|----------|------------------|
| **Unix** | 进程模型、syscall 接口 |
| **BSD** | 按需分页、高性能 TCP/IP 栈思路 |
| **Solaris** | **VFS**、**Slab 分配器**、**DTrace**、ZFS |

**要点：** 今天 Linux 里的 VFS、slab、观测文化，很多来自这条演进线 — 不是「Linux 独有发明一切」。

→ 设计脉络：[a01 Unix 设计基因](../02-Linux-Kernel-Development/03_Course_Kernel_Architecture/episode-a01-Unix设计基因.md)、[a02 宏内核 vs 微内核](../02-Linux-Kernel-Development/03_Course_Kernel_Architecture/episode-a02-宏内核与微内核.md)（待补）

---

### Linux 性能相关里程碑

| 引入 | 作用 |
|------|------|
| **O(1) / CFS 调度器** | 可扩展调度 |
| **RCU** | 读多写少路径低争用同步 |
| **epoll** | 高并发 I/O 多路复用（相对 select/poll） |
| **cgroups** | 资源隔离与限额 |
| **THP（透明大页）** | 减少 TLB miss，亦有延迟抖动争议 |
| **KVM** | 硬件虚拟化（共置 vs 云） |

---

### Linux 现代性能焦点（三件）

#### 1. systemd

- 现代服务管理器；**`systemd-analyze`** 可分解**启动时间**（哪 unit 慢）。
- HFT 裸机：关注 **服务依赖、After=、是否拖慢共置机器就绪**；热路径进程常不由 systemd 频繁重启。

#### 2. KPTI（Meltdown 缓解）

- **内核页表隔离**：修复 CPU 侧信道漏洞，增加 **syscall / 上下文切换** 时的页表与 **TLB 刷新** 开销。
- 影响：**约 0.1%–6%**（ workload 依赖）；syscall 密集或切换频繁时更明显。
- HFT：评估是否可用 **PCID**、内核版本、mitigations 开关（与安全合规权衡）→ 与 [10-HFT ch05](../10-HFT-Low-Latency-Practice/chapter-05-操作系统内核极致调优.md) 对照。

#### 3. Extended BPF（eBPF）

- 内核态 **安全虚拟机**：验证后运行，**可编程观测**（tracepoint、kprobe、uprobe、XDP/tc-BPF…）。
- 驱动 **BCC、bpftrace** 等 — Gregg 称「当前最重要技术之一」。
- **全书观测主线：** [Ch 15 BPF](./chapter-15-BPF技术.md) → [09-BPF-Performance-Tools](../09-BPF-Performance-Tools/)

```
Ch 3 知道「BPF 能在内核里安全插桩」
  → Ch 4 选工具
  → Ch 15 + 09 系统学
```

---

## 3.5 其他系统模型

| 模型 | 思路 | 与宏内核关系 |
|------|------|--------------|
| **PGO 内核** | 用 CPU 剖析数据 **配置文件引导编译**，热路径布局更优 | Linux 可选 profile-guided 构建 |
| **Unikernel** | 应用与内核 **编译为一体**，减抽象层、减 syscall | 专用场景；牺牲通用性换延迟 |
| **微内核 / 混合内核** | 最小内核 + 用户态服务；或 Mach + BSD 层（如早期 macOS） | Linux 为 **宏内核**；对比见架构课 a02 |

**HFT 现实：** 主流仍是 **Linux 宏内核 + 绑核/旁路（DPDK/XDP）**；Unikernel 多见于研究或极专用部署。

→ 架构对比：[episode-a02](../02-Linux-Kernel-Development/03_Course_Kernel_Architecture/episode-a02-宏内核与微内核.md)

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
3. 确认 **swap 关闭 / 关键内存 mlock**；THP 策略与 [note-THP](../03-Linux-Virtual-Memory-Manager/note-透明大页THP.md) 一致。
4. 装 bpftrace，跑一条 **syscall 计数**（预告 Ch 15）验证热路径 syscall 量。

---

## 相关章节

- 上一章：[chapter-02-方法论.md](./chapter-02-方法论.md)
- 下一章：[chapter-04-观测工具.md](./chapter-04-观测工具.md)
- 应用程序：[chapter-05-应用程序.md](./chapter-05-应用程序.md)
- CPU / 内存：[chapter-06-中央处理器.md](./chapter-06-中央处理器.md) · [chapter-07-内存.md](./chapter-07-内存.md)
- 网络：[chapter-10-网络.md](./chapter-10-网络.md)
- BPF：[chapter-15-BPF技术.md](./chapter-15-BPF技术.md)
- 内核架构课：[02/03_Course_Kernel_Architecture](../02-Linux-Kernel-Development/03_Course_Kernel_Architecture/)
- 全书目录：[OUTLINE.md](./OUTLINE.md)
