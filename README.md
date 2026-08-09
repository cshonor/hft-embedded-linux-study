# hft-embedded-linux-study

> **GitHub：** [github.com/cshonor/hft-embedded-linux-study](https://github.com/cshonor/hft-embedded-linux-study)  
> **HFT 低延迟 Linux 底层** + **嵌入式 Linux 无人机飞控** 双线笔记与路线仓库。

**技术板块 `00`–`23`（含现代补充 `08.5` / `08.6` / `09.5` / `17.5`）：** 顶层为**纯技术模块名**；**整数编号 = 学习顺序**，`.5` / `.6` = 现代内核补充资料（见 [§现代内核补充](#现代内核补充资料5--6-模块)）。

---

## 相关仓库

| 仓库 | 用途 | 本仓对应 |
|------|------|----------|
| **[hft-embedded-linux-study](https://github.com/cshonor/hft-embedded-linux-study)** | 本仓：读序、OUTLINE、章节 scaffold | `00`–`23` |
| **[cpp-learning-notes](https://github.com/cshonor/cpp-learning-notes)** | C / C++ 详细笔记与代码 | [01 C](./01-c-language/) · [06 C++](./06-cpp/) |
| [Computer-Networking](https://github.com/cshonor/Computer-Networking) | Socket / UNP / TCP/IP 实战代码 | [15 network-sockets](./15-network-sockets/) |

```bash
git clone https://github.com/cshonor/hft-embedded-linux-study.git
```

---

## 技术模块总览（编号 = 读序）

> **递进主轴：** 硬件底层 → 编程语言 → Linux 系统 → 驱动/设备树 → 嵌入式工程 → 网络栈 → 性能工具 → HFT 上层业务

| # | 文件夹 | 定位 | Phase |
|---|--------|------|:-----:|
| **00** | [digital-logic-cpu](./00-digital-logic-cpu/) | 硬件底层：组合/时序/CPU 词汇 | **1** 当前 |
| **01** | [c-language](./01-c-language/) | C / 指针 / GNU-C | 2 |
| **02** | [computer-systems](./02-computer-systems/) | 程序=机器：栈/缓存/VM/并发 | 2 |
| **03** | [computer-architecture](./03-computer-architecture/) | 体系结构加深（拓展） | 6 |
| **04** | [linux-userspace-api](./04-linux-userspace-api/) | 用户态系统编程（TLPI） | 3 |
| **05** | [os-from-scratch](./05-os-from-scratch/) | 自制 OS 动手 | 3 穿插 |
| **06** | [cpp](./06-cpp/) | C++（Modern / 并发 / 对象模型） | 3 穿插 |
| **07** | [linux-kernel](./07-linux-kernel/) | 内核入门（LKD） | 4 |
| **08.5** | [modern-kernel](./08.5-modern-kernel/) | 现代 5.x/6.x 内核**非 MM** 资料（补 ULK/LKD 2.6 过时） | 4 |
| **08** | [linux-kernel-deep](./08-linux-kernel-deep/) | 内核深度 ULK3（拓展） | 6 拓展 |
| **08.6** | [kernel-debugging](./08.6-kernel-debugging/) | 内核正确性调试（KASAN/KGDB/Ftrace） | 4 |
| **09** | [linux-mm](./09-linux-mm/) | 内核内存管理（Gorman） | 4 |
| **09.5** | [modern-mm](./09.5-modern-mm/) | 现代 5.x/6.x **MM** 资料（补 Gorman 2.4/2.6 过时） | 4 |
| **10** | [arm-architecture](./10-arm-architecture/) | ARM / AArch64 | 5A |
| **11** | [embedded-boot-build](./11-embedded-boot-build/) | U-Boot / 内核构建 / rootfs | 5A |
| **12** | [device-drivers-dt](./12-device-drivers-dt/) | 驱动 + 设备树 | 5A |
| **13** | [embedded-projects](./13-embedded-projects/) | 板级 / 无人机 / 网关实战 | 5A |
| **14** | [motion-control](./14-motion-control/) | PID / 姿态 / 飞控（兴趣） | 5A/6 |
| **15** | [network-sockets](./15-network-sockets/) | Socket 编程（UNP / PNP） | 5B |
| **16** | [tcpip-protocols](./16-tcpip-protocols/) | TCP/IP 协议（Stevens 卷一） | 5B |
| **17** | [kernel-networking](./17-kernel-networking/) | 内核网络栈（Rosen） | 5B |
| **17.5** | [modern-networking](./17.5-modern-networking/) | 现代 5.x/6.x **网络** 资料（补 Rosen 3.x 过时） | 5B |
| **18** | [dpdk](./18-dpdk/) | 用户态高速网络（DPDK） | 5B |
| **19** | [systems-performance](./19-systems-performance/) | 系统性能方法论（Gregg） | 5B |
| **20** | [bpf-observability](./20-bpf-observability/) | BPF / 可观测（Gregg） | 5B |
| **21** | [hft-engineering](./21-hft-engineering/) | HFT 工程实践 | 5B |
| **22** | [rust-quant](./22-rust-quant/) | Rust 量化（拓展） | 6 |
| **23** | [markets-microstructure](./23-markets-microstructure/) | 交易 / 微观结构（业务） | 6 |

---

## 学习路线（Phase 顺序 · 锁定）

> **结论：** 书单深度够、广度闭环；**不要再扩书**。成败在于 **自底向上顺序** + **动手 Demo**。  
> **重要：** **文件夹编号 = 学习顺序**（`00` → `23`）。书名只出现在各模块 README / `refs/`。

```
Phase1  00 数字逻辑/CPU（当前；未完成前不正式开下一 Phase）
   ↓
Phase2  01 C → 02 计算机系统
   ↓
Phase3  04 用户态 API（穿插 05 自制 OS / 06 C++）
   ↓
Phase4  07 内核入门 → 08.5 现代内核 → 08.6 调试 → 09 MM → 09.5 现代 MM
        （08 ULK 深度可后补）
   ↓
Phase5  分叉并行
        A 嵌入式: 10 → 11 → 12 → 13（14 兴趣）
        B HFT:    15 → 16 → 17 → 17.5 → 18 → 19 → 20 → 21
   ↓
Phase6  拓展: 03 · 08 · 22 · 23 ·（兴趣）14
```

| Phase | 内容 | 过关感 |
|-------|------|--------|
| **1** | `00` 数字逻辑/CPU（黑盒语义为主） | setup/hold、寄存器与 FIFO；不纠结门级 |
| **2** | `01` C → `02` 计算机系统 | 指针/内存过关；流水线、Cache、VM、并发能讲通 |
| **3** | `04` → 穿插 `05`/`06` | 进程/线程/信号/`mmap`/`epoll`；能写小 Demo |
| **4** | `07` → `08.5` → `08.6` → `09` → `09.5` | 调度、内存、同步入门地图清晰；知道 6.x 现代实现 |
| **5A** | `10`–`13` | 启动链、设备树、简单驱动、板级闭环 |
| **5B** | `15`–`21`（含 `17.5`） | Socket → 协议 → 内核网 → 现代 Net → DPDK → 观测 → HFT |
| **6** | 拓展书/业务 | 主线闭环后再加 |

### 深度约束（已定）

- `00`：组合/时序取黑盒语义；门级/Verilog 不主攻 → 见 `00-…/学习深度_*.md`
- `02`：流水线/缓存/VM 为主粮；Ch4 是 Y86+HCL，不是 Verilog

### 必须警惕

1. **禁止乱跳**：未完成 Phase1/2 不要冲内核、DPDK、HFT。  
2. **时间不均分**：电机、Rust 量化、体系结构加深前期少投入。  
3. **必须动手**：无锁队列、绑核、大页、简易 UDP；嵌入式侧编译内核、设备树调试。  
4. **少开并行文件夹**：优先啃透当前 Phase。

---

## 现代内核补充资料（`.5` / `.6` 模块）

> 经典内核书基于旧版内核（ULK/LKD → 2.6，Gorman → 2.4/2.6，Rosen → 3.x），**设计思想可借鉴，但大量结构体/函数/算法在 6.x 已重构**。`.5` / `.6` 模块用**笨叔《奔跑吧 Linux 内核》+ LWN.net + Bootlin 讲义**补齐时代差异。

| 模块 | 补谁的过时 | 资料来源 | 学完做什么 |
|------|-----------|----------|-----------|
| **08.5** modern-kernel | ULK3/LKD3（2.6 非 MM 部分） | 笨叔(调度/RCU/ARM64) + LWN + Bootlin | 进 `08` ULK 源码阅读前先建立 6.x 认知 |
| **08.6** kernel-debugging | —（Kaiwan《Linux Kernel Debugging》2022, 5.x） | printk/Kprobes/KASAN/KGDB/Ftrace/Lockdep | 内核模块**正确性**调试；与 19/20 形成"正确性→性能→可观测" |
| **09.5** modern-mm | Gorman（2.4/2.6 MM） | 笨叔卷1(MM) + LWN(SLUB/folio/MGLRU/5级页表) + Bootlin | 进 `09` 源码阅读前先建立 6.x MM 认知 |
| **17.5** modern-networking | Rosen（3.x 网络） | LWN(XDP/eBPF/io_uring/NAPI) + 内核文档 + Bootlin | 进 `18` DPDK 前先建立 6.x 网络栈认知 |

> ⚠️ **禁止直接拿旧书 API 对照 6.x 源码**：bootmem→memblock、SLAB→SLUB、highmem 在 ARM64 不存在、LRU→MGLRU、page→folio、Netfilter→nftables、无 XDP/eBPF 网络。

**学习流转模式（以内核为例）：**

```
07 LKD（建概念框架，不照搬代码）
   ↓
08.5 现代内核（5.x/6.x 真实实现，非 MM）
   ↓
08 ULK3（源码深度阅读 + 模块实验）↔ 08.6 调试（出 bug 怎么修）
```

---

## 跨模块联动

### 网络学习链（推荐顺序）

```
00 数字逻辑 → 01 C → 02 计算机系统
    ↓
04 用户态 API → 05 自制 OS → 06 C++
    ↓
07 内核 + 09 MM（+ 08.5 / 09.5 现代补充）
    ↓
15 sockets → 16 TCP/IP → 17 内核网络 → 17.5 现代网络 → 18 DPDK
    ↓
19 SysPerf → 20 BPF → 21 HFT
```

### 内核网络栈 vs 用户态旁路

| 对比项 | 内核栈（15 / 17 / 17.5） | 用户态旁路（18 DPDK） |
|--------|--------------------------|----------------------|
| 收包触发 | 中断 + NAPI 软中断 | 用户态 busy-poll |
| 缓冲结构 | `sk_buff`（现代 `page_pool` / `xdp_buff`） | `rte_mbuf` |
| 系统调用 | `recvfrom` / `epoll_wait` | 无（UIO/VFIO） |

### 内核：正确性 → 性能 → 可观测

| 模块 | 核心问题 | 工具层级 |
|------|----------|----------|
| **08.6** kernel-debugging | 内核为什么**坏了** | KASAN/KGDB/Kprobes（需重编译） |
| **19** systems-performance | 系统为什么**慢了** | perf/top/Ftrace（低侵入） |
| **20** bpf-observability | 内核**正在做什么** | bpftrace/BCC（运行时注入） |

> 完整链路：先保证正确性（08.6）→ 再优化性能（19）→ 最后持续观测（20）。

### 嵌入式支线（`10`–`14`）

与 HFT 主线在 Phase4 后分叉；**定位：第二职业退路**（飞行器/网关/车载），仅 **ARM-A + 嵌入式 Linux**，**不学** STM32/MCU 裸机/FreeRTOS 飞控/PCB。详见 [HFT-READING-ROADMAP §六](./HFT-READING-ROADMAP.md)。

---

## 必读书目（精读清单摘要）

> 标签：🔴 必读（直接作用于热路径/延迟/撮合） · 🟡 选读（有上下文价值） · ⚪ 跳过（与 HFT 无关）  
> **分章精读 + HFT 标签** → [READING-LIST.md](./READING-LIST.md) · [HFT-READING-ROADMAP.md](./HFT-READING-ROADMAP.md)

| # | 书 | 模块 | HFT 关联 |
|---|-----|------|----------|
| 1 | Systems Performance 2nd — Gregg | `19` | 延迟分解、perf、NUMA、网卡调优总纲 |
| 2 | Linux Kernel Development 3rd — Love | `07` | 调度、中断、CFS、绑核底层 |
| 2b | Understanding the Linux Kernel 3rd — Bovet | `08` | LKD 功能 ↔ 源码实现的桥梁 |
| 3 | Understanding the Linux VM Manager — Gorman | `09` | slab、THP、NUMA、伪共享 |
| 4 | Linux Kernel Networking — Rosen | `17` | sk_buff、NAPI、组播内核路径 |
| 5 | Computer Architecture 6th — Hennessy | `03` | Cache line、MESI、memory order |
| 6 | CSAPP 3rd — Bryant | `02` | 缓存/VM/并发/网络编程程序员落地 |
| 7 | Trading and Exchanges — Harris | `23` | LOB、撮合、市场微观结构 |
| 8 | BPF Performance Tools — Gregg | `20` | eBPF、XDP、生产观测 |
| 12 | DPDK（官方文档 + 深入浅出 DPDK） | `18` | PMD、mbuf、零拷贝旁路 |
| — | The Linux Programming Interface — Kerrisk | `04` | epoll、mmap、mlock、RT 调度 |
| — | Linux Kernel Debugging — Billimoria | `08.6` | KASAN/KGDB/Ftrace 内核正确性调试 |
| 外C | C++ 学习链（Primer→Effective→Concurrency） | `06` | M1 Modern C++ / M2 并发+对象模型 |
| 外P | 陈硕 PNP / muduo | `15` | epoll 多路复用实验骨架 |
| 外B | UNP Vol.1 — Stevens | `15` | Socket API、TCP_NODELAY、非阻塞 |
| 外A | TCP/IP Illustrated Vol.1 — Stevens | `16` | UDP/组播、IP 分片、TCP |

> **不要整本迁入本仓库。** 外部书目（PNP/UNP/TCP-IP）笔记留在 [Computer-Networking](https://github.com/cshonor/Computer-Networking)，本仓库做**索引 + HFT 裁剪清单**。

---

## Project 驱动学习路线

> 不是"先读完书再做项目"，而是**项目本身就是学习路径**——卡住了翻书查对应模块，做完就自然学会了。  
> 项目脚手架 → [`projects/`](./projects/)

```
P1 CPU 模拟器 → P2 Shell+malloc → P2.5 C 工具箱 → P3 并发 HTTP Server → P3.5 BusyBox 极简 Linux
 → P4 内核模块
 → P5 树莓派嵌入式（5 子项目）
 → P6 网络协议分析器 → P7 DPDK 转发+延迟剖析
 → P8 迷你撮合引擎（终极大作业）
```

| Project | 做什么 | 覆盖模块 | 前置 | 脚手架 |
|:-------:|--------|:--------:|:----:|--------|
| **P1** | Logisim/Verilog 搭 8-bit CPU（ALU+寄存器+FSM） | `00` | 无 | [projects/P1-cpu-simulator](./projects/P1-cpu-simulator/) |
| **P2** | C 写 mini shell（fork/exec/pipe）+ 自制 malloc/free + C 特性练手 | `01` `02` | P1 | [projects/P2-shell-malloc](./projects/P2-shell-malloc/) |
| **P2.5** | GNU C 工具箱：container_of + 侵入式链表 + 无锁 ring buffer + vtable | `01` | P2 | [projects/P2.5-c-toolkit](./projects/P2.5-c-toolkit/) |
| **P3** | 并发 HTTP Server：C 版（epoll+线程池）→ C++ 重写版（RAII+模板） | `04` `05` `06` | P2 | [projects/P3-http-server](./projects/P3-http-server/) |
| **P3.5** | BusyBox 极简 Linux：内核编译 + rootfs + QEMU 启动到 shell | `07` `11` | P3 | [projects/P3.5-busybox-minimal-linux](./projects/P3.5-busybox-minimal-linux/) |
| **P4** | 可加载内核模块：字符设备 + kmalloc 追踪 + /proc 统计 | `07` `08.5` `08.6` `09` | P3+P3.5+P2.5 | [projects/P4-kernel-module](./projects/P4-kernel-module/) |
| **P5** | 树莓派嵌入式 Linux 全链路（5 子项目见下） | `10`–`14` | P4 | [projects/P5-raspberry-pi-embedded](./projects/P5-raspberry-pi-embedded/) |
| **P6** | raw socket 抓包 + 逐层解析 + TCP 流重组 + eBPF 追踪 NAPI | `15` `16` `17` `17.5` `20` | P3 | [projects/P6-network-protocol-analyzer](./projects/P6-network-protocol-analyzer/) |
| **P7** | DPDK packet forwarder + perf 火焰图 + bpftrace 延迟探针 | `18` `19` `20` | P6 | [projects/P7-dpdk-forwarder-profiling](./projects/P7-dpdk-forwarder-profiling/) |
| **P8** | 限价订单簿撮合引擎：无锁 ring buffer + 绑核/Hugepage + Rust 重写 | `21` `22` `23` | P4+P5+P7 | [projects/P8-matching-engine](./projects/P8-matching-engine/) |

### P5 子项目（树莓派嵌入式）

| 子项目 | 交付 | 模块 | 脚手架 |
|:------:|------|:----:|--------|
| P5a | QEMU 裸机 UART Hello World | `10` | [P5a-qemu-uart-hello](./projects/P5-raspberry-pi-embedded/P5a-qemu-uart-hello/) |
| P5b | U-Boot → kernel → rootfs 启动到 shell | `11` | [P5b-uboot-kernel-rootfs](./projects/P5-raspberry-pi-embedded/P5b-uboot-kernel-rootfs/) |
| P5c | I2C/SPI 传感器驱动 + 设备树 | `12` | [P5c-i2c-spi-driver-dt](./projects/P5-raspberry-pi-embedded/P5c-i2c-spi-driver-dt/) |
| P5d | 多线程传感器融合 + 延迟 p99 统计 | `13` | [P5d-sensor-fusion-latency](./projects/P5-raspberry-pi-embedded/P5d-sensor-fusion-latency/) |
| P5e | PID 姿态控制（可选） | `14` | [P5e-pid-attitude-control](./projects/P5-raspberry-pi-embedded/P5e-pid-attitude-control/) |

---

## 当前状态

- **正在：** Phase1 · `00-digital-logic-cpu`
- **下一站：** Phase2 · `01-c-language` → `02-computer-systems`
- **暂不新开：** `07`/`17`/`18`/`21` 等（除非做极小对照实验）
- **板卡动手清单（Pi5）：** [13-embedded-projects/RASPBERRY-PI5-LABS.md](./13-embedded-projects/RASPBERRY-PI5-LABS.md)（A→G 执行序；官方文档当工具书）

---

## 详细参考（深链）

README 已是完整入口；以下文件保留**分章精读细节**，按需深入：

| 文件 | 内容 |
|------|------|
| [HFT-READING-ROADMAP.md](./HFT-READING-ROADMAP.md) | 分章精读路线 + HFT 不漏项检查清单 + 嵌入式支线详情 |
| [READING-LIST.md](./READING-LIST.md) | 9 本 + 外部书目章节级精读/选读/跳过标签 |
