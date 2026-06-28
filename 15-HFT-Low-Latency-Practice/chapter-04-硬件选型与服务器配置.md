# 第4章 HFT 系统基础：硬件到操作系统

> **原书第 4 章 · HFT System Foundations – From Hardware to OS**  
> **NUMA · 缓存 · OS 抽象代价 · 编译/链接**

← [chapter-03 交易所动态](./chapter-03-订单簿深度与行情解析.md) · 实操调优：[chapter-05](./chapter-05-操作系统内核极致调优.md)

---

## 本章定位

宏观架构（Ch1–3）之后，**视线拉回底层**：代码如何在 **CPU、内存、内核** 中运行。

| 核心思想 | **打破抽象，直面物理** |
|----------|------------------------|
| 目标 | **Tick-to-Trade** 达 μs/ns — **仅靠高级语言算法不够** |
| 手段 | 迎合 **缓存** · 减 **分页/TLB 惩罚** · 剔除热点上的 **OS 干预** |

**本章 = 原理**；**Linux 绑核 / Bypass / Hugepage 命令** → [chapter-05](./chapter-05-操作系统内核极致调优.md)

---

## 1. 硬件：商用服务器榨干物理极限

HFT **通常不需定制机** — **现成 X86 服务器** + **配置与代码** 决胜负。

### NUMA（非统一内存访问）

```
Socket 0 ── local RAM ── CPU 0,1,…
    ╲
     ╲ QPI/UPI（跨 socket 慢）
    ╱
Socket 1 ── local RAM ── CPU …
```

| 规则 | 说明 |
|------|------|
| **本地内存快** | 访 **远端 socket** 内存 → **高延迟** |
| **HFT 设计** | **NIC 收包线程** 与 **网卡** 绑 **同一 NUMA node** |
| **工具** | `numactl --membind` · `lstopo` |

### 超线程（Hyper-threading）陷阱

- 硬件 **交替** 两逻辑核共享执行单元 → **吞吐↑**  
- **牺牲** 软件对调度的控制 → **Jitter↑**  
- **BIOS：关 HT** — 热点线程 **一物理核一线程**

### 缓存金字塔（Cache Hierarchy）

| 层级 | 典型 | 延迟量级 |
|------|------|----------|
| **L1I / L1D** | 每核私有 | 最快 |
| **L2** | 每核 |  |
| **L3** | 全核共享 |  |
| **RAM** | 相对 CPU **极慢** |  |

**Cache-friendly 代码：**

- **连续内存**（vector/ring）> 指针追逐链表  
- **热数据结构** 控制在 **L1/L2 工作集**  
- **False sharing** — 多线程改 **同一 cache line** → [chapter-07](./chapter-07-无锁数据结构与内存布局.md)

→ [02-Computer-Architecture-6th](../02-Computer-Architecture-6th/) · [09 MikanOS Ch8/19 分页](../09-system-low-level-hands-on/02-mikan-os/chapter-19-paging/)

---

## 2. 操作系统：便利抽象 vs HFT 累赘

### User Space vs Kernel Space

| 空间 | 权限 | HFT 热点 |
|------|------|----------|
| **用户态** | 低 | Gateway / Book / Strategy |
| **内核态** | 高 — 控硬件 | **syscall 进出昂贵** |

每次 **`recv()`/`send()`** → **模式切换** + 内核协议栈 — 见 Ch5 **Kernel Bypass** 规避。

→ [08-TLPI](../08-The-Linux-Programming-Interface/) · [MikanOS Ch20 syscall](../09-system-low-level-hands-on/02-mikan-os/chapter-20-syscall/)

### 上下文切换（Context Switch）

OS 暂停线程 A、运行 B 时：

1. **保存** A 寄存器/状态  
2. **加载** B 状态  
3. **清空/污染** CPU 缓存 → B **冷启动 Cache Miss**

**对策：** **绑核 + isolcpus** — 热点线程 **独占核**，拒绝公平调度 → [chapter-05 §1](./chapter-05-操作系统内核极致调优.md#1-cpu-pinning-与隔离)

### 内存管理与大页（Huge Pages）

- **页表** 映射 VA→PA  
- **TLB** 缓存映射 — **Miss** 则 **内存 walk**，极慢  
- **4KB → 2MB/1GB 大页** — **TLB 覆盖更多内存** → 少 Miss  

原理在此；挂载与 `nr_hugepages` → [chapter-05 §4](./chapter-05-操作系统内核极致调优.md#4-内存pool--huge-pages)

### 中断（Interruption）

网卡收包 → **硬件 IRQ** → **打断** 用户线程。

| 模式 | 特点 |
|------|------|
| **中断驱动** | 低负载友好 · **高 PPS 时 IRQ 风暴** |
| **NAPI / 轮询（DPDK/OpenOnload）** | 用户态 **poll** — 热点 **可预测** |

→ [chapter-06 网络](./chapter-06-低延迟网络与协议优化.md) · [14-DPDK](../14-DPDK-Low-Latency-Network/)

---

## 3. 编译器与链接

### 编译期优化

| 优化 | 作用 |
|------|------|
| **Loop unrolling** | 减分支与循环控制 |
| **Function inlining** | 消除 call 开销 · 助 **常量传播** |
| **LTO / -O3 -march=native** | 目标 CPU 指令集 |

**配合 C++：** 模板/CRTP 让 **多态在编译期** — [chapter-08 §5](./chapter-08-超低延迟核心引擎开发.md#5-c-引擎编码规范热点路径)

### 静态链接 vs 动态链接

| | **Static** | **Dynamic (.so)** |
|---|------------|-------------------|
| **部署** | 单二进制 | 共享库省磁盘 |
| **调用** | 直接地址 | **PLT 间接跳转** |
| **HFT** | **首选** — 跨库 **LTO** · 无运行时解析 |

```bash
# 示意
g++ -O3 -static -o gateway main.cpp ...
```

---

## 4. 硬件选型速查（工程）

| 项 | 建议 |
|----|------|
| **CPU** | 高 **单核频率** · 少跨 NUMA |
| **BIOS** | 关 HT / Turbo / C-states |
| **NIC** | 硬件时间戳 · Bypass 生态（Solarflare/DPDK） |
| **FPGA** | 软件 **1–5 μs** 不够时 — ns 级确定性 |

### FPGA（纳秒级）

| 下沉 | 说明 |
|------|------|
| MD 解码 / 轻量 trigger | 无 OS 调度 |
| T2T | 可 **<500 ns** |

→ [chapter-01 §6 FPGA](./chapter-01-高频交易基础与生态.md#6-fpga纳秒级)

---

## 本章小结

| 层 | 要点 |
|----|------|
| **CPU/NUMA** | NIC 线程 **同 node** |
| **HT** | **关** — 控 Jitter |
| **Cache** | 连续结构 · 小工作集 |
| **OS** | 少 syscall · 少切换 · 少 IRQ · **大页** |
| **构建** | **-O3 静态链接** |

**打破抽象，直面物理** — 下一章落地 Linux：**[chapter-05 OS 极致调优](./chapter-05-操作系统内核极致调优.md)** · 网络：**[chapter-06](./chapter-06-低延迟网络与协议优化.md)**
