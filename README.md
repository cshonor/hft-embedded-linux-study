# hft-embedded-linux-study

> **GitHub：** [github.com/cshonor/hft-embedded-linux-study](https://github.com/cshonor/hft-embedded-linux-study)  
> **HFT 低延迟 Linux 底层** + **嵌入式 Linux 无人机飞控** 双线笔记与路线仓库。

**技术板块 `00`–`22`（含 `.5` 模块 `03.5` / `04.5` / `05.5` / `05.6` / `06.5` / `12.5` / `13.5`）：** 顶层为**纯技术模块名**；**整数编号 = 学习顺序**，`.5` / `.6` = 现代补充资料或语言衔接（见 [§现代补充资料](#现代补充资料5--6-模块)）。

---

## 相关仓库

| 仓库 | 用途 | 本仓对应 |
|------|------|----------|
| **[hft-embedded-linux-study](https://github.com/cshonor/hft-embedded-linux-study)** | 本仓：读序、OUTLINE、章节 scaffold | `00`–`22` |
| **[cpp-learning-notes](https://github.com/cshonor/cpp-learning-notes)** | C / C++ 详细笔记与代码 | [01 C](./01-c-language/) · [04 C++](./04-cpp/) |
| [Computer-Networking](https://github.com/cshonor/Computer-Networking) | Socket / UNP / TCP/IP 实战代码 | [03.5 UNP](./03.5-unix-network-api/) · [04.5 network-sockets](./04.5-network-sockets/) |

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
| **03** | [linux-userspace-api](./03-linux-userspace-api/) | 用户态系统编程（TLPI） | 3 |
| **03.5** | [unix-network-api](./03.5-unix-network-api/) | Socket API 精读（UNP — Stevens） | 3 |
| **04** | [cpp](./04-cpp/) | C++（Modern / 并发 / 对象模型） | 3 穿插 |
| **04.5** | [network-sockets](./04.5-network-sockets/) | C++ 网络编程（muduo / PNP） | 3 穿插 |
| **05** | [linux-kernel](./05-linux-kernel/) | 内核入门（LKD） | 4 |
| **05.5** | [modern-kernel](./05.5-modern-kernel/) | 现代 5.x/6.x 内核**非 MM** 资料（补 ULK/LKD 2.6 过时） | 4 |
| **05.6** | [kernel-debugging](./05.6-kernel-debugging/) | 内核正确性调试（KASAN/KGDB/Ftrace） | 4 |
| **06** | [linux-mm](./06-linux-mm/) | 内核内存管理（Gorman） | 4 |
| **06.5** | [modern-mm](./06.5-modern-mm/) | 现代 5.x/6.x **MM** 资料（补 Gorman 2.4/2.6 过时） | 4 |
| **07** | [arm-architecture](./07-arm-architecture/) | ARM / AArch64 | 5A |
| **08** | [embedded-boot-build](./08-embedded-boot-build/) | U-Boot / 内核构建 / rootfs | 5A |
| **09** | [device-drivers-dt](./09-device-drivers-dt/) | 驱动 + 设备树 | 5A |
| **10** | [embedded-projects](./10-embedded-projects/) | 板级 / 无人机 / 网关实战 | 5A |
| **11** | [motion-control](./11-motion-control/) | PID / 姿态 / 飞控（兴趣） | 5A/6 |
| **12** | [tcpip-protocols](./12-tcpip-protocols/) | TCP/IP 协议（Stevens 卷一） | 5B |
| **12.5** | [wireshark-packet-analysis](./12.5-wireshark-packet-analysis/) | 抓包分析实战 | 5B |
| **13** | [kernel-networking](./13-kernel-networking/) | 内核网络栈（Rosen） | 5B |
| **13.5** | [modern-networking](./13.5-modern-networking/) | 现代 5.x/6.x **网络** 资料（补 Rosen 3.x 过时） | 5B |
| **14** | [dpdk](./14-dpdk/) | 用户态高速网络（DPDK） | 5B |
| **15** | [systems-performance](./15-systems-performance/) | 系统性能方法论（Gregg） | 5B |
| **16** | [bpf-observability](./16-bpf-observability/) | BPF / 可观测（Gregg） | 5B |
| **17** | [hft-engineering](./17-hft-engineering/) | HFT 工程实践 | 5B |
| **18** | [computer-architecture](./18-computer-architecture/) | 体系结构加深（拓展） | 6 |
| **19** | [linux-kernel-deep](./19-linux-kernel-deep/) | 内核深度 ULK3（拓展） | 6 |
| **20** | [rust-foundation](./20-rust-foundation/) | Rust 基础（拓展） | 6 |
| **21** | [rust-quant](./21-rust-quant/) | Rust 量化（拓展） | 6 |
| **22** | [markets-microstructure](./22-markets-microstructure/) | 交易 / 微观结构（业务） | 6 |

---

## 学习路线（Phase 顺序 · 锁定）

> **结论：** 书单深度够、广度闭环；**不要再扩书**。成败在于 **自底向上顺序** + **动手 Demo**。  
> **重要：** **文件夹编号 = 学习顺序**（`00` → `22`）。书名只出现在各模块 README / `refs/`。

```
Phase1  00 数字逻辑/CPU（当前；未完成前不正式开下一 Phase）
   ↓
Phase2  01 C → 02 计算机系统
   ↓
Phase3  03 用户态 API → 03.5 UNP socket → 04 C++ → 04.5 muduo 网络编程
   ↓
Phase4  05 内核入门 → 05.5 现代内核 → 05.6 调试 → 06 MM → 06.5 现代 MM
        （20 ULK 深度可后补）
   ↓
Phase5  分叉并行
        A 嵌入式: 07 → 08 → 09 → 10（11 兴趣）
        B HFT:    12 → 12.5 → 13 → 13.5 → 14 → 15 → 16 → 17
   ↓
Phase6  拓展: 18 · 19 · 20 · 21 · 22 · P9(OS from scratch) ·（兴趣）11
```

| Phase | 内容 | 过关感 |
|-------|------|--------|
| **1** | `00` 数字逻辑/CPU（黑盒语义为主） | setup/hold、寄存器与 FIFO；不纠结门级 |
| **2** | `01` C → `02` 计算机系统 | 指针/内存过关；流水线、Cache、VM、并发能讲通 |
| **3** | `03` → `03.5` → 穿插 `04` → `04.5` | 进程/线程/信号/`mmap`/`epoll`；能写小 Demo |
| **4** | `05` → `05.5` → `05.6` → `06` → `06.5` | 调度、内存、同步入门地图清晰；知道 6.x 现代实现 |
| **5A** | `07`–`10` | 启动链、设备树、简单驱动、板级闭环 |
| **5B** | `12`–`17`（含 `12.5` 和 `13.5`） | Socket → 协议 → 内核网 → 现代 Net → DPDK → 观测 → HFT |
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

## 现代补充资料（`.5` / `.6` 模块）

> 经典内核书基于旧版内核（ULK/LKD → 2.6，Gorman → 2.4/2.6，Rosen → 3.x），**设计思想可借鉴，但大量结构体/函数/算法在 6.x 已重构**。`.5` / `.6` 模块用**笨叔《奔跑吧 Linux 内核》+ LWN.net + Bootlin 讲义**补齐时代差异。

| 模块 | 补谁的过时 | 资料来源 | 学完做什么 |
|------|-----------|----------|-----------|
| **05.5** modern-kernel | ULK3/LKD3（2.6 非 MM 部分） | 笨叔(调度/RCU/ARM64) + LWN + Bootlin | 进 `19` ULK 源码阅读前先建立 6.x 认知 |
| **05.6** kernel-debugging | —（Kaiwan《Linux Kernel Debugging》2022, 5.x） | printk/Kprobes/KASAN/KGDB/Ftrace/Lockdep | 内核模块**正确性**调试；与 15/16 形成"正确性→性能→可观测" |
| **06.5** modern-mm | Gorman（2.4/2.6 MM） | 笨叔卷1(MM) + LWN(SLUB/folio/MGLRU/5级页表) + Bootlin | 进 `06` 源码阅读前先建立 6.x MM 认知 |
| **13.5** modern-networking | Rosen（3.x 网络） | LWN(XDP/eBPF/io_uring/NAPI) + 内核文档 + Bootlin | 进 `14` DPDK 前先建立 6.x 网络栈认知 |

> ⚠️ **禁止直接拿旧书 API 对照 6.x 源码**：bootmem→memblock、SLAB→SLUB、highmem 在 ARM64 不存在、LRU→MGLRU、page→folio、Netfilter→nftables、无 XDP/eBPF 网络。

**学习流转模式（以内核为例）：**

```
05 LKD（建概念框架，不照搬代码）
   ↓
05.5 现代内核（5.x/6.x 真实实现，非 MM）
   ↓
19 ULK3（源码深度阅读 + 模块实验）↔ 05.6 调试（出 bug 怎么修）
```

---

## 跨模块联动

### 网络学习链（推荐顺序）

```
00 数字逻辑 → 01 C → 02 计算机系统
    ↓
03 用户态 API → 03.5 UNP socket → 04 C++ → 04.5 muduo 网络编程
    ↓
05 内核 + 06 MM（+ 05.5 / 06.5 现代补充）
    ↓
12 TCP/IP → 12.5 抓包 → 13 内核网络 → 13.5 现代网络 → 14 DPDK
    ↓
15 SysPerf → 16 BPF → 17 HFT
```

### 内核网络栈 vs 用户态旁路

| 对比项 | 内核栈（12 / 13 / 13.5） | 用户态旁路（14 DPDK） |
|--------|--------------------------|----------------------|
| 收包触发 | 中断 + NAPI 软中断 | 用户态 busy-poll |
| 缓冲结构 | `sk_buff`（现代 `page_pool` / `xdp_buff`） | `rte_mbuf` |
| 系统调用 | `recvfrom` / `epoll_wait` | 无（UIO/VFIO） |

### 内核：正确性 → 性能 → 可观测

| 模块 | 核心问题 | 工具层级 |
|------|----------|----------|
| **05.6** kernel-debugging | 内核为什么**坏了** | KASAN/KGDB/Kprobes（需重编译） |
| **15** systems-performance | 系统为什么**慢了** | perf/top/Ftrace（低侵入） |
| **16** bpf-observability | 内核**正在做什么** | bpftrace/BCC（运行时注入） |

> 完整链路：先保证正确性（05.6）→ 再优化性能（15）→ 最后持续观测（16）。

### 嵌入式支线（`07`–`11`）

与 HFT 主线在 Phase4 后分叉；**定位：第二职业退路**（飞行器/网关/车载），仅 **ARM-A + 嵌入式 Linux**，**不学** STM32/MCU 裸机/FreeRTOS 飞控/PCB。详见 [HFT-READING-ROADMAP §六](./HFT-READING-ROADMAP.md)。

---

## 必读书目（精读清单摘要）

> 标签：🔴 必读（直接作用于热路径/延迟/撮合） · 🟡 选读（有上下文价值） · ⚪ 跳过（与 HFT 无关）  
> **分章精读 + HFT 标签** → [READING-LIST.md](./READING-LIST.md) · [HFT-READING-ROADMAP.md](./HFT-READING-ROADMAP.md)

| # | 书 | 模块 | HFT 关联 |
|---|-----|------|----------|
| 1 | Systems Performance 2nd — Gregg | `15` | 延迟分解、perf、NUMA、网卡调优总纲 |
| 2 | Linux Kernel Development 3rd — Love | `05` | 调度、中断、CFS、绑核底层 |
| 2b | Understanding the Linux Kernel 3rd — Bovet | `19` | LKD 功能 ↔ 源码实现的桥梁 |
| 3 | Understanding the Linux VM Manager — Gorman | `06` | slab、THP、NUMA、伪共享 |
| 4 | Linux Kernel Networking — Rosen | `13` | sk_buff、NAPI、组播内核路径 |
| 5 | Computer Architecture 6th — Hennessy | `18` | Cache line、MESI、memory order |
| 6 | CSAPP 3rd — Bryant | `02` | 缓存/VM/并发/网络编程程序员落地 |
| 7 | Trading and Exchanges — Harris | `22` | LOB、撮合、市场微观结构 |
| 8 | BPF Performance Tools — Gregg | `16` | eBPF、XDP、生产观测 |
| 12 | DPDK（官方文档 + 深入浅出 DPDK） | `14` | PMD、mbuf、零拷贝旁路 |
| — | The Linux Programming Interface — Kerrisk | `03` | epoll、mmap、mlock、RT 调度 |
| — | Linux Kernel Debugging — Billimoria | `05.6` | KASAN/KGDB/Ftrace 内核正确性调试 |
| 外C | C++ 学习链（Primer→Effective→Concurrency） | `04` | M1 Modern C++ / M2 并发+对象模型 |
| 外P | 陈硕 PNP / muduo | `04.5` | epoll 多路复用实验骨架 |
| 外B | UNP Vol.1 — Stevens | `03.5` | Socket API、TCP_NODELAY、非阻塞 |
| 外A | TCP/IP Illustrated Vol.1 — Stevens | `12` | UDP/组播、IP 分片、TCP |

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
 → P8 迷你撮合引擎 → P10 HFT 单机原型（终局项目）
```

| Project | 做什么 | 覆盖模块 | 前置 | 脚手架 |
|:-------:|--------|:--------:|:----:|--------|
| **P1** | Logisim/Verilog 搭 8-bit CPU（ALU+寄存器+FSM） | `00` | 无 | [projects/P1-cpu-simulator](./projects/P1-cpu-simulator/) |
| **P2** | C 写 mini shell（fork/exec/pipe）+ 自制 malloc/free + C 特性练手 | `01` `02` | P1 | [projects/P2-shell-malloc](./projects/P2-shell-malloc/) |
| **P2.5** | GNU C 工具箱：container_of + 侵入式链表 + 无锁 ring buffer + vtable | `01` | P2 | [projects/P2.5-c-toolkit](./projects/P2.5-c-toolkit/) |
| **P3** | 并发 HTTP Server：C 版（epoll+线程池）→ C++ 重写版（RAII+模板） | `03` `04` | P2 | [projects/P3-http-server](./projects/P3-http-server/) |
| **P3.5** | BusyBox 极简 Linux：内核编译 + rootfs + QEMU 启动到 shell | `05` `08` | P3 | [projects/P3.5-busybox-minimal-linux](./projects/P3.5-busybox-minimal-linux/) |
| **P4** | 可加载内核模块：字符设备 + kmalloc 追踪 + /proc 统计 | `05` `05.5` `05.6` `06` | P3+P3.5+P2.5 | [projects/P4-kernel-module](./projects/P4-kernel-module/) |
| **P5** | 树莓派嵌入式 Linux 全链路（5 子项目见下） | `07`–`11` | P4 | [projects/P5-raspberry-pi-embedded](./projects/P5-raspberry-pi-embedded/) |
| **P6** | raw socket 抓包 + 逐层解析 + TCP 流重组 + eBPF 追踪 NAPI | `04.5` `12` `13` `13.5` `16` | P3 | [projects/P6-network-protocol-analyzer](./projects/P6-network-protocol-analyzer/) |
| **P7** | DPDK packet forwarder + perf 火焰图 + bpftrace 延迟探针 | `14` `15` `16` | P6 | [projects/P7-dpdk-forwarder-profiling](./projects/P7-dpdk-forwarder-profiling/) |
| **P8** | 限价订单簿撮合引擎：无锁 ring buffer + 绑核/Hugepage + Rust 重写 | `17` `21` `22` | P4+P5+P7 | [projects/P8-matching-engine](./projects/P8-matching-engine/) |
| **P10** | HFT 单机原型：DPDK 行情 + 撮合引擎 + 策略 + 风控 + 回测完整链路 | `14` `17` `18` `22` | P7+P8 | [projects/P10-hft-prototype](./projects/P10-hft-prototype/) |

### P5 子项目（树莓派嵌入式）

| 子项目 | 交付 | 模块 | 脚手架 |
|:------:|------|:----:|--------|
| P5a | QEMU 裸机 UART Hello World | `07` | [P5a-qemu-uart-hello](./projects/P5-raspberry-pi-embedded/P5a-qemu-uart-hello/) |
| P5b | U-Boot → kernel → rootfs 启动到 shell | `08` | [P5b-uboot-kernel-rootfs](./projects/P5-raspberry-pi-embedded/P5b-uboot-kernel-rootfs/) |
| P5c | I2C/SPI 传感器驱动 + 设备树 | `09` | [P5c-i2c-spi-driver-dt](./projects/P5-raspberry-pi-embedded/P5c-i2c-spi-driver-dt/) |
| P5d | 多线程传感器融合 + 延迟 p99 统计 | `10` | [P5d-sensor-fusion-latency](./projects/P5-raspberry-pi-embedded/P5d-sensor-fusion-latency/) |
| P5e | PID 姿态控制（可选） | `11` | [P5e-pid-attitude-control](./projects/P5-raspberry-pi-embedded/P5e-pid-attitude-control/) |

---

## 当前状态

- **正在：** Phase1 · `00-digital-logic-cpu`
- **下一站：** Phase2 · `01-c-language` → `02-computer-systems`
- **暂不新开：** `05`/`13`/`14`/`17` 等（除非做极小对照实验）
- **板卡动手清单（Pi5）：** [10-embedded-projects/RASPBERRY-PI5-LABS.md](./10-embedded-projects/RASPBERRY-PI5-LABS.md)（A→G 执行序；官方文档当工具书）

---

## 详细参考（深链）

README 已是完整入口；以下文件保留**分章精读细节**，按需深入：

| 文件 | 内容 |
|------|------|
| [HFT-READING-ROADMAP.md](./HFT-READING-ROADMAP.md) | 分章精读路线 + HFT 不漏项检查清单 + 嵌入式支线详情 |
| [READING-LIST.md](./READING-LIST.md) | 9 本 + 外部书目章节级精读/选读/跳过标签 |
