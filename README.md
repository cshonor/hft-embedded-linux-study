# hft-embedded-linux-study

> **GitHub：** [github.com/cshonor/hft-embedded-linux-study](https://github.com/cshonor/hft-embedded-linux-study)
> **HFT 低延迟 Linux 底层** + **嵌入式 Linux 无人机飞控** 双线笔记与路线仓库。

**技术板块 `00`–`19`（含 `.5` / `.6` / `.7` 模块 `03.5` / `03.6` / `05.5` / `05.6` / `06.5` / `06.6` / `06.7` / `11.5` / `12.5`）：** 顶层为**纯技术模块名**；**整数编号 = 学习顺序**，`.5`–`.7` = 现代补充资料或性能/可观测衔接（见 [§现代补充资料](#现代补充资料5--6-模块)）。

---

## 文档地图（先看这张表）

仓库根目录只有 3 份路线文档，**职责不重叠**。不确定看哪份时按「回答什么问题」这一列走：

| 文档 | 回答什么问题 | 什么时候看 |
|------|--------------|-----------|
| **README.md**（本页） | 学什么顺序、做什么项目、读到哪了 | **默认入口**，别的不用先翻 |
| [READING-LIST.md](./READING-LIST.md) | 某本书**具体哪几章**该精读/选读/跳过 | 开始啃某本书之前 |
| [HFT-READING-ROADMAP.md](./HFT-READING-ROADMAP.md) | 分书小节级指引 + **HFT 不漏项检查清单** + 嵌入式支线详情 | 怀疑自己有知识缺口时对照 |
| [14-hft-engineering/HFT-ENGINEERING-LADDER.md](./14-hft-engineering/HFT-ENGINEERING-LADDER.md) | L0–L5 每级**交付什么项目、验收指标是多少** | 准备动手写代码时 |

> **12 个月时间表**已于 2026-09-06 并入本页 [§12 个月时间表](#12-个月时间表可选参考)，不再单开文件——原文件用的是**已废弃的旧编号体系**（DPDK 标 `15`、SysPerf 标 `16`、HFT 标 `18`，还出现不存在的 `14.5` / `22`），与当前目录全错位，留着只会误导。

---

## 相关仓库

| 仓库 | 用途 | 本仓对应 |
|------|------|----------|
| **[hft-embedded-linux-study](https://github.com/cshonor/hft-embedded-linux-study)** | 本仓：读序、OUTLINE、章节 scaffold | `00`–`19` |
| **[cpp-learning-notes](https://github.com/cshonor/cpp-learning-notes)** | C / C++ 详细笔记与代码 | [01 C](./01-c-language/) · [04 C++](./04-cpp/) |
| [Computer-Networking](https://github.com/cshonor/Computer-Networking) | Socket / UNP / TCP/IP 实战代码 | [03.5 UNP](./03.5-unix-network-api/) · [04/M2 C++ 网络编程](./04-cpp/M2-cpp-network-programming/) |

```bash
git clone https://github.com/cshonor/hft-embedded-linux-study.git
```

---

## 技术模块总览（编号 = 读序）

> **递进主轴：** 硬件底层 → 编程语言 → Linux 系统 → 驱动/设备树 → 嵌入式工程 → 网络栈 → 性能工具 → HFT 上层业务

| # | 文件夹 | 定位 | Phase |
|---|--------|------|:-----:|
| **00** | [digital-logic-cpu](./00-digital-logic-cpu/) | 硬件底层：组合/时序/CPU 词汇 | **1** |
| **01** | [c-language](./01-c-language/) | C / 指针 / GNU-C | 2 |
| **02** | [computer-systems](./02-computer-systems/) | 程序=机器：栈/缓存/VM/并发 | 2 |
| **03** | [linux-userspace-api](./03-linux-userspace-api/) | 用户态系统编程（TLPI） | 3 |
| **03.5** | [unix-network-api](./03.5-unix-network-api/) | Socket API 精读（UNP — Stevens） | 3 |
| **03.6** | [userspace-debugging](./03.6-userspace-debugging/) | 用户态调试（gdb / strace / valgrind / ASan / TSan） | 3 |
| **04** | [cpp](./04-cpp/) | C++（Modern / 并发 / 对象模型；M5 = **C++ 网络编程** muduo·PNP） | 3 穿插 |
| **05** | [linux-kernel](./05-linux-kernel/) | 内核入门（LKD） | 4 |
| **05.5** | [modern-kernel](./05.5-modern-kernel/) | 现代 5.x/6.x 内核**非 MM** 资料（补 ULK/LKD 2.6 过时） | 4 |
| **05.6** | [kernel-debugging](./05.6-kernel-debugging/) | 内核正确性调试（KASAN/KGDB/Ftrace） | 4 |
| **06** | [linux-mm](./06-linux-mm/) | 内核内存管理（Gorman） | 4 |
| **06.5** | [modern-mm](./06.5-modern-mm/) | 现代 5.x/6.x **MM** 资料（补 Gorman 2.4/2.6 过时） | 4 |
| **07** | [arm-architecture](./07-arm-architecture/) | ARM / AArch64 | 5A |
| **08** | [embedded-boot-build](./08-embedded-boot-build/) | U-Boot / 内核构建 / rootfs | 5A |
| **09** | [device-drivers-dt](./09-device-drivers-dt/) | 驱动 + 设备树 | 5A |
| **10** | [motion-control](./10-motion-control/) | PID / 姿态 / 飞控（兴趣；板级实战并入 [P5](./projects/P5-raspberry-pi-embedded/)） | 5A/6 |
| **11** | [tcpip-protocols](./11-tcpip-protocols/) | TCP/IP 协议（Stevens 卷一） | 5B |
| **11.5** | [wireshark-packet-analysis](./11.5-wireshark-packet-analysis/) | 抓包分析实战 | 5B |
| **12** | [kernel-networking](./12-kernel-networking/) | 内核网络栈（Rosen） | 5B |
| **12.5** | [modern-networking](./12.5-modern-networking/) | 现代 5.x/6.x **网络** 资料（补 Rosen 3.x 过时） | 5B |
| **13** | [dpdk](./13-dpdk/) | 用户态高速网络（DPDK） | 5B |
| **06.6** | [systems-performance](./06.6-systems-performance/) | 系统性能方法论（Gregg） | 5B |
| **06.7** | [bpf-observability](./06.7-bpf-observability/) | BPF / 可观测（Gregg） | 5B |
| **14** | [hft-engineering](./14-hft-engineering/) | HFT 工程实践 | 5B |
| **15** | [computer-architecture](./15-computer-architecture/) | 体系结构加深（拓展） | 6 |
| **16** | [linux-kernel-deep](./16-linux-kernel-deep/) | 内核深度 ULK3（拓展） | 6 |
| **17** | [rust-foundation](./17-rust-foundation/) | Rust 基础（拓展） | 6 |
| **18** | [rust-quant](./18-rust-quant/) | Rust 量化（拓展） | 6 |
| **19** | [markets-microstructure](./19-markets-microstructure/) | 交易 / 微观结构（业务） | 6 |

---

## 学习路线（Phase 顺序 · 锁定）

> **结论：** 书单深度够、广度闭环；**不要再扩书**。成败在于 **自底向上顺序** + **动手 Demo**。
> **重要：** **文件夹编号 = 学习顺序**（`00` → `19`）。书名只出现在各模块 README / `refs/`。

```
Phase1  00 数字逻辑/CPU（当前；未完成前不正式开下一 Phase）
   ↓
Phase2  01 C → 02 计算机系统
   ↓
Phase3  03 用户态 API → 03.5 UNP socket → 03.6 用户态调试 → 04 C++ → M5 muduo 网络编程（04 内）
   ↓
Phase4  05 内核入门 → 05.5 现代内核 → 05.6 调试 → 06 MM → 06.5 现代 MM
        （16 ULK 深度可后补）
   ↓
Phase5  分叉并行
        A 嵌入式: 07 → 08 → 09 → P5 板级实战（10 兴趣）
        B HFT:    11 → 11.5 → 12 → 12.5 → 13 → 06.6 → 06.7 → 14
   ↓
Phase6  拓展: 15 · 16 · 17 · 18 · 19 · P9(OS from scratch) ·（兴趣）10
```

| Phase | 内容 | 过关感 |
|-------|------|--------|
| **1** | `00` 数字逻辑/CPU（黑盒语义为主） | setup/hold、寄存器与 FIFO；不纠结门级 |
| **2** | `01` C → `02` 计算机系统 | 指针/内存过关；流水线、Cache、VM、并发能讲通 |
| **3** | `03` → `03.5` → `03.6` → 穿插 `04`（含 M5 muduo） | 进程/线程/信号/`mmap`/`epoll`；能写小 Demo；会用 gdb/strace/valgrind 调自己的代码 |
| **4** | `05` → `05.5` → `05.6` → `06` → `06.5` | 调度、内存、同步入门地图清晰；知道 6.x 现代实现 |
| **5A** | `07`–`09` + P5 | 启动链、设备树、简单驱动、板级闭环 |
| **5B** | `11`–`14`（含 `11.5` 和 `12.5`） | Socket → 协议 → 内核网 → 现代 Net → DPDK → 观测 → HFT |
| **6** | 拓展书/业务 | 主线闭环后再加 |

### 深度约束（已定）

- `00`：组合/时序取黑盒语义；门级/Verilog 不主攻 → 见 `00-…/学习深度_*.md`
- `02`：流水线/缓存/VM 为主粮；Ch4 是 Y86+HCL，不是 Verilog

### 必须警惕

1. **禁止乱跳**：未完成 Phase1/2 不要冲内核、DPDK、HFT。
2. **时间不均分**：电机、Rust 量化、体系结构加深前期少投入。
3. **必须动手**：无锁队列、绑核、大页、简易 UDP；嵌入式侧编译内核、设备树调试。
4. **少开并行文件夹**：优先啃透当前 Phase。

#### 12 个月时间表（可选参考）

<details>
<summary><b>展开：月度映射 · 检查点 · 原型 vs 生产边界</b></summary>

> 上表 Phase 是**顺序**约束，本表是**时间**映射。两者冲突时以 Phase 为准——基础没过就顺延，不要为赶月份跳级。
> **目标：** 单机 HFT 技术原型（DPDK + 无锁订单簿 + 内存池 + 撮合引擎）。**不是**可上实盘的生产系统。

| 月 | Phase | 模块 | 项目 | 过关标准 |
|:--:|:-----:|------|------|----------|
| M1 | 1–2 | `00` → `01` | P1 | 能手写 `container_of`；能解释 cache line 伪共享 |
| M2 | 2–3 | `01` → `02` · `04`/M0–M1 | P2 · P2.5 | C → C++ 切换完成，能写 RAII 风格并发代码 |
| M3 | 3 | `03` TLPI · `03.5` UNP | P3 | `epoll`/`mmap`/`fork`/`pthread` 能徒手写 Demo |
| M4 | 3–4 | `03.6` · `05` · `05.5` · `05.6` · `06` · `06.5` | P3.5 · P4 | 能写可加载内核模块；能讲通 page fault → buddy → slab |
| M5 | 5A | `07` · `08` · `09` · `10` | P5a–P5e | Pi 从 U-Boot 启动到 shell；写过真实设备树驱动 |
| M6 | 5B | `11` · `11.5` · `12` · `12.5` | P6 | 能画 网卡 → NAPI → `sk_buff` → socket → 用户态 收包路径 |
| M7 | 5B | `13` DPDK · `06.6` · `06.7` | P7 | DPDK 收发包跑通；能用 perf + bpftrace 定位热路径 |
| M8 | 5B | `14` · `15` · `19` | — | 手写过无锁 SPSC ring；能用 RDTSC 做纳秒级延迟测量 |
| M9 | — | `14` · `19` | P8 | 订单簿能正确撮合；内存池零动态分配；绑核跑通 |
| M10 | — | `14` · `19` | P8 | 完整链路跑通：收行情 → 更新 LOB → 生成信号 → 下单 → 统计 PnL |
| M11 | — | `14` | P8 | 端到端延迟有量化数据；基础风控覆盖异常路径 |
| M12 | — | — | P8 收尾 | 百万级回放压测 + 架构文档 + benchmark；15 分钟讲清架构 |

**关键检查点：**

| 月份 | 检查项 | 如果没达到 |
|:----:|--------|-----------|
| M2 末 | C/C++ 基本功过关，P1+P2+P2.5 完成 | 延后 Phase 3，不硬冲 |
| M4 末 | 能写内核模块，理解 MM 链路 | 延后网络模块 |
| M7 末 | DPDK 收发包跑通，perf/bpftrace 熟练 | 不开撮合引擎 |
| M10 末 | 撮合引擎完整链路跑通 | 砍风控复杂度，保链路完整 |
| M12 末 | 原型完成 + 文档 + benchmark | 即使简化也要有可展示的交付 |

**原型 vs 生产系统（边界认知）：**

| 维度 | 本路线产出（原型） | 机构级生产系统 |
|------|-------------------|---------------|
| 行情接入 | 模拟器 / WebSocket | 交易所专线 UDP 组播 + 二进制协议 |
| 风控 | 基础仓位/重复检测 | 成千上万边界 + 实时熔断 + 多层校验 |
| 容错 | 单机，crash 即停 | 双机热备 + 故障切换 + 状态恢复 |
| 时间同步 | RDTSC / `clock_gettime` | 硬件 PTP + 多源比对 + 坏数据剔除 |
| 合规 | 无 | 交易所认证 + 审计日志 + 监管对接 |
| 压测 | 百万级回放 | 千万级 + 线上持续 + 概率性 bug 复现 |

> 原型的价值在于**学习全链路技术 + 求职展示**，不是直接上实盘。

</details>

---

## 动手线：能力阶梯 + 项目

学习路线管「读什么」，这一节管「做什么」。**不是先读完书再做项目，而是项目本身就是路径**——卡住了翻书查对应模块。

### 能力阶梯 L0 → L5

> 完整版（每级知识点表 + 交付细节 + 硬验收指标）→ [14-hft-engineering/HFT-ENGINEERING-LADDER.md](./14-hft-engineering/HFT-ENGINEERING-LADDER.md)
> **你当前位置：L0**（《Pointers on C》§7.1 stream model / FILE 对象，与 L0 的「自实现 `malloc`」正好咬合）

| 级 | 核心知识 | 交付项目 | 硬验收指标 |
|:--:|----------|----------|------------|
| **L0** | 指针算术、struct 布局、堆分配、ABI | 自实现 `malloc` + 对齐/合并 benchmark | 能解释 chunk header / bins / `M_MMAP_THRESHOLD` |
| **L1** | TLPI：fd、线程、`mmap`、信号、`epoll` | 多线程 TCP echo server（epoll ET + 线程池） | p99 < 200μs；能画出请求完整路径 |
| **L2** | LKD + Gorman：调度/中断/VMA/页表/slab | `perf` 定位并消除一次真实抖动 | 能用火焰图 + `perf stat` 说清瓶颈归属 |
| **L3** | 组播、UDP、socket 选项、NAPI | UDP 组播行情接收器（含丢包统计） | 10 万 pps 下**零丢包**，能说出丢包在哪一层 |
| **L4** | cache line / NUMA / 内存序 / 无锁 | SPSC 无锁 ring（padding 前后对比） | 单跳 < 100ns，p99 < 200ns，**批量消费**版更快 |
| **L5** | DPDK/AF_XDP、LOB、PTP、T2T 测量 | 三进程：FeedHandler → Book → Strategy | 软件栈 tick-to-trade **p99 < 10μs**（自测环境如实记录） |

> **ARM64 汇编在 L4 第一次变现：** x86 是 TSO 强序，`acquire/release` 编译成零指令——所以「忘了写 `memory_order`」在 x86 上常常碰巧能跑；ARM64 弱序，`ldar`/`stlr` 少一条就直接崩。能解释这个差别 = 真懂内存序，而不是背了六个枚举。

### 项目路线 P1 → P10

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
| **P5** | 树莓派嵌入式 Linux 全链路（6 子项目见下） | `07`–`10` + P5 Labs | P4 | [projects/P5-raspberry-pi-embedded](./projects/P5-raspberry-pi-embedded/) |
| **P6** | raw socket 抓包 + 逐层解析 + TCP 流重组 + eBPF 追踪 NAPI | `04/M2` `11` `12` `12.5` `06.7` | P3 | [projects/P6-network-protocol-analyzer](./projects/P6-network-protocol-analyzer/) |
| **P7** | DPDK packet forwarder + perf 火焰图 + bpftrace 延迟探针 | `13` `06.6` `06.7` | P6 | [projects/P7-dpdk-forwarder-profiling](./projects/P7-dpdk-forwarder-profiling/) |
| **P8** | 限价订单簿撮合引擎：无锁 ring buffer + 绑核/Hugepage + Rust 重写 | `14` `18` `19` | P4+P5+P7 | [projects/P8-matching-engine](./projects/P8-matching-engine/) |
| **P10** | HFT 单机原型：DPDK 行情 + 撮合引擎 + 策略 + 风控 + 回测完整链路 | `13` `14` `15` `19` | P7+P8 | [projects/P10-hft-prototype](./projects/P10-hft-prototype/) |

<details>
<summary><b>P5 子项目（树莓派嵌入式）</b></summary>

| 子项目 | 交付 | 模块 | 脚手架 |
|:------:|------|:----:|--------|
| P5a | QEMU 裸机 UART Hello World | `07` | [P5a-qemu-uart-hello](./projects/P5-raspberry-pi-embedded/P5a-qemu-uart-hello/) |
| P5b | U-Boot → kernel → rootfs 启动到 shell | `08` | [P5b-uboot-kernel-rootfs](./projects/P5-raspberry-pi-embedded/P5b-uboot-kernel-rootfs/) |
| P5c | I2C/SPI 传感器驱动 + 设备树 | `09` | [P5c-i2c-spi-driver-dt](./projects/P5-raspberry-pi-embedded/P5c-i2c-spi-driver-dt/) |
| P5d | 多线程传感器融合 + 延迟 p99 统计 | P5 Labs | [P5d-sensor-fusion-latency](./projects/P5-raspberry-pi-embedded/P5d-sensor-fusion-latency/) |
| P5e | PID 姿态控制（可选） | `10` | [P5e-pid-attitude-control](./projects/P5-raspberry-pi-embedded/P5e-pid-attitude-control/) |

</details>

---

## 必读书目（精炼清单）

> 标签：🔴 必读（直接作用于热路径/延迟/撮合） · 🟡 选读（有上下文价值） · ⚪ 跳过（与 HFT 无关）
> **分章精读 + HFT 标签** → [READING-LIST.md](./READING-LIST.md) · [HFT-READING-ROADMAP.md](./HFT-READING-ROADMAP.md)

| # | 书 | 模块 | HFT 关联 |
|---|-----|------|----------|
| 1 | Systems Performance 2nd — Gregg | `06.6` | 延迟分解、perf、NUMA、网卡调优总纲 |
| 2 | Linux Kernel Development 3rd — Love | `05` | 调度、中断、CFS、绑核底层 |
| 2b | Understanding the Linux Kernel 3rd — Bovet | `16` | LKD 功能 ↔ 源码实现的桥梁 |
| 3 | Understanding the Linux VM Manager — Gorman | `06` | slab、THP、NUMA、伪共享 |
| 4 | Linux Kernel Networking — Rosen | `12` | sk_buff、NAPI、组播内核路径 |
| 5 | Computer Architecture 6th — Hennessy | `15` | Cache line、MESI、memory order |
| 6 | CSAPP 3rd — Bryant | `02` | 缓存/VM/并发/网络编程程序员落地 |
| 7 | Trading and Exchanges — Harris | `19` | LOB、撮合、市场微观结构 |
| 8 | BPF Performance Tools — Gregg | `06.7` | eBPF、XDP、生产观测 |
| 12 | DPDK（官方文档 + 深入浅出 DPDK） | `13` | PMD、mbuf、零拷贝旁路 |
| — | The Linux Programming Interface — Kerrisk | `03` | epoll、mmap、mlock、RT 调度 |
| — | Linux Kernel Debugging — Billimoria | `05.6` | KASAN/KGDB/Ftrace 内核正确性调试 |
| 外C | C++ 学习链（Primer→Effective→Concurrency） | `04` | M1 Modern C++ / M3 并发+对象模型 |
| 外P | 陈硕 PNP / muduo | `04/M2` | epoll 多路复用实验骨架 |
| 外B | UNP Vol.1 — Stevens | `03.5` | Socket API、TCP_NODELAY、非阻塞 |
| 外A | TCP/IP Illustrated Vol.1 — Stevens | `11` | UDP/组播、IP 分片、TCP |

**HFT 原版专题书目**（7 本，章节级裁剪表见 [READING-LIST §9](./READING-LIST.md#9-hft-原版专题书目低延迟工程--微观结构--策略数学--纪实)）：

| 书 | 读序 | 备注 |
|----|:----:|------|
| Developing HFT Systems — Donadio / Ghosh / Rossier | **L1–L2** | **= `14` 模块原书**（ch06 = 原书 Ch5 *Networking in Motion*，ch08 = Ch8 *C++ Quest for Microsecond Latency*） |
| Building Low Latency Applications with C++ — Sourav Ghosh | L3–L4 | 12 章完整章节级裁剪，TOC 已核对（2023 / 506 页） |
| Low-Latency C++ Programming — Antony Williams | L4 | 主题清单（官方 TOC 未检索到，章号未核对） |
| High-Frequency Trading — Irene Aldridge | L4 后 | 业务视角：做市、统计套利、延迟套利 |
| Algorithmic and HFT — Cartea / Jaimungal / Penalva | 后置 | 随机最优控制；**做底层开发可暂缓** |
| Flash Boys — Michael Lewis | 任意 | 无代码，行业背景 |

> **不要整本迁入本仓库。** 外部书目（PNP/UNP/TCP-IP）笔记留在 [Computer-Networking](https://github.com/cshonor/Computer-Networking)，本仓库做**索引 + HFT 裁剪清单**。

---

## 现代补充资料（`.5` / `.6` 模块）

> 经典内核书基于旧版内核（ULK/LKD → 2.6，Gorman → 2.4/2.6，Rosen → 3.x），**设计思想可借鉴，但大量结构体/函数/算法在 6.x 已重构**。`.5` / `.6` 模块用**笨叔《奔跑吧 Linux 内核》+ LWN.net + Bootlin 讲义**补齐时代差异。

| 模块 | 补谁的过时 | 资料来源 | 学完做什么 |
|------|-----------|----------|-----------|
| **05.5** modern-kernel | ULK3/LKD3（2.6 非 MM 部分） | 笨叔(调度/RCU/ARM64) + LWN + Bootlin | 进 `16` ULK 源码阅读前先建立 6.x 认知 |
| **05.6** kernel-debugging | —（Kaiwan《Linux Kernel Debugging》2022, 5.x） | printk/Kprobes/KASAN/KGDB/Ftrace/Lockdep | 内核模块**正确性**调试；与 06.6/06.7 形成"正确性→性能→可观测" |
| **06.5** modern-mm | Gorman（2.4/2.6 MM） | 笨叔卷1(MM) + LWN(SLUB/folio/MGLRU/5级页表) + Bootlin | 进 `06` 源码阅读前先建立 6.x MM 认知 |
| **12.5** modern-networking | Rosen（3.x 网络） | LWN(XDP/eBPF/io_uring/NAPI) + 内核文档 + Bootlin | 进 `13` DPDK 前先建立 6.x 网络栈认知 |

> ⚠️ **禁止直接拿旧书 API 对照 6.x 源码**：bootmem→memblock、SLAB→SLUB、highmem 在 ARM64 不存在、LRU→MGLRU、page→folio、Netfilter→nftables、无 XDP/eBPF 网络。

**学习流转模式（以内核为例）：**

```
05 LKD（建概念框架，不照搬代码）
   ↓
05.5 现代内核（5.x/6.x 真实实现，非 MM）
   ↓
16 ULK3（源码深度阅读 + 模块实验）↔ 05.6 调试（出 bug 怎么修）
```

> 📌 **`.5` 是「现代补丁层」，不是「第 5 个子模块」**
> `05.5` 与 `05.6` 是**两本不同的书**，编号只是挨着；同理 `06.5` vs `06.6`。
> → 这几对编号**不存在合并关系**，合并等于把两本无关的书塞进一个目录。

<details>
<summary><b>按主题查：该翻哪一层（30 个高频主题扫描结论）</b></summary>

> 绝大多数主题主场清晰，少数几处已用双向链接打通。查东西按这张表走，不用两层都翻。

| 主题 | 主场（查这里） | 另一侧的情况 |
|------|--------------|------------|
| qspinlock | **05.5** 专题 | `05` Ch10.2 有 55 处（自旋锁章节必然要讲）— **双向已通** |
| maple tree / XArray | **06.5** 专题 | `05` Ch6.6 只讲**选型维度**（46 / 33 处）— **双向已通** |
| per-VMA lock | **06.5** 专题 | `05` Ch9.6 只讲**争用测量** — **双向已通** |
| memblock | **06.5** | ⚠️ 见下方警示 — **双向已通** |
| SLUB | **06.5** | `05` Ch12.7 讲原理与接口（47 处），`06` 也有 93 处 — **双向已通** |
| folio | **06.5**（4 个专题） | `06` 有 147 处但**全分散顺带提及**（无单篇 ≥20）→ 健康分工 |
| THP | **`06` Ch3 专题** | 06.5 无专题（仅 4 处提及），原书也未专章 |
| MGLRU / DAMON / zswap / PSI | **06.5**（专题） | `06` 仅顺带提及 |
| EEVDF 调度 | **05.5**（82 处） | `05` 10 处 |
| XDP / AF_XDP / page_pool | **12.5**（1734 / 240 / 231） | `12` 仅个位数（原书基于 3.x，早于 XDP） |
| NAPI | **12.5**（02 / 03 专题） | `12` 有 36 处 |
| nftables / tc-BPF | **12.5**（85 / 82） | `12` 有 8 / 0 |
| io_uring | **12.5**（121） | `05` 有 84 处（作为 syscall 演进提及） |

> ⚠️ **最容易踩的一个**：`06` Ch5「启动内存分配器」原书讲的是 **bootmem**，而 bootmem 在 **v4.x 后已彻底移除**。想查取代它的 `memblock` 要去 **06.5**。
> 🔑 **判定「撞车」的方法**：看**有没有专题文件**，不是看关键词总命中次数。例如 folio 在 `06`（147）与 `06.5`（179）总数接近，但 `06.5` 有 4 个专题而 `06` 里**没有任何单篇 ≥20 次** —— 这是健康分工，不是重复。

</details>

---

## 跨模块联动

### 网络学习链（推荐顺序）

```
00 数字逻辑 → 01 C → 02 计算机系统
    ↓
03 用户态 API → 03.5 UNP socket → 04 C++ → M5 muduo 网络编程（04 内）
    ↓
05 内核 + 06 MM（+ 05.5 / 06.5 现代补充）
    ↓
11 TCP/IP → 11.5 抓包 → 12 内核网络 → 12.5 现代网络 → 13 DPDK
    ↓
06.6 SysPerf → 06.7 BPF → 14 HFT
```

### 内核网络栈 vs 用户态旁路

| 对比项 | 内核栈（11 / 12 / 12.5） | 用户态旁路（13 DPDK） |
|--------|--------------------------|----------------------|
| 收包触发 | 中断 + NAPI 软中断 | 用户态 busy-poll |
| 缓冲结构 | `sk_buff`（现代 `page_pool` / `xdp_buff`） | `rte_mbuf` |
| 系统调用 | `recvfrom` / `epoll_wait` | 无（UIO/VFIO） |

### 内核：正确性 → 性能 → 可观测

| 模块 | 核心问题 | 工具层级 |
|------|----------|----------|
| **05.6** kernel-debugging | 内核为什么**坏了** | KASAN/KGDB/Kprobes（需重编译） |
| **06.6** systems-performance | 系统为什么**慢了** | perf/top/Ftrace（低侵入） |
| **06.7** bpf-observability | 内核**正在做什么** | bpftrace/BCC（运行时注入） |

> 完整链路：先保证正确性（05.6）→ 再优化性能（06.6）→ 最后持续观测（06.7）。

### 嵌入式支线（`07`–`10`）

与 HFT 主线在 Phase4 后分叉；**定位：第二职业退路**（飞行器/网关/车载），仅 **ARM-A + 嵌入式 Linux**，**不学** STM32/MCU 裸机/FreeRTOS 飞控/PCB。详见 [HFT-READING-ROADMAP §六](./HFT-READING-ROADMAP.md#六嵌入式-linux-支线0710)。

---

## 当前状态

- **正在（多线并行）：**
  - `03` TLPI 逐章精读笔记（主线，64 章推进中）
  - `05` LKD 薄笔记扩写：批 A–H 已收官（Ch1/5/6/12 内存/15 进程地址空间等），剩 Ch10 同步方法 8 篇 + 散落 7 篇
  - `09` Madieu 设备驱动 Ch12 DMA 扩展精读：12.0 基础 / 12.1 缓存一致性 / 12.2 映射 API / 12.3 scatter-gather 已完成，12.4 DMA Engine / 12.5 DTS 绑定待写
  - `06.7` eBPF 双书笔记（Learning eBPF / BPF Performance Tools，含 ch08 XDP 增强）
  - `01` Pointers on C（Ch7 stream model / FILE 对象）
- **下一站：** Madieu 12.4 / 12.5 · LKD Ch10 收官 · TLPI 后续章节
- **板卡动手清单（Pi5）：** [projects/P5-raspberry-pi-embedded/RASPBERRY-PI5-LABS.md](./projects/P5-raspberry-pi-embedded/RASPBERRY-PI5-LABS.md)（A→G 执行序；官方文档当工具书）

---

## HTML 阅读版（单文件聚合页 · 全仓 59 本）

> 每章一份单文件 HTML，**CFS 同款暗色主题** + 侧边栏目录 + 进度条 + scrollspy + 自测题折叠。离线可读，手机/平板/打印机都方便。
> **全部 59 本书已按 11 个领域分组**，每本一个封面入口 → 章节页/附录页。
> 在线浏览：[GitHub Pages](https://cshonor.github.io/hft-embedded-linux-study/) · **顶层总目录** → [html/index.html](./html/index.html)
> 站内**每个文件夹都有自动生成的目录导航页**（文件树式逐级浏览）。

<details>
<summary><b>59 本 HTML 入口索引（按领域分组 · 点击展开）</b></summary>

### C 语言（6 本）

| 书 | 定位 | HTML 入口 | 页数 |
|---|------|-----------|:----:|
| The C Programming Language | K&R · 8 章 + 3 附录精读 | [html](./01-c-language/01-Primer-K-and-R-C/html/index.html) | 19 |
| Pointers on C | Kenneth Reek · 指针与 C 精读 | [html](./01-c-language/02-Pointers-on-C/html/index.html) | 19 |
| Expert C Programming | van der Linden · 深 C 语言 | [html](./01-c-language/03-Advanced-Expert-C-Programming/html/index.html) | 13 |
| Modern C | Jens Gustedt · 现代 C 精读 | [html](./01-c-language/04-Modern-C/html/index.html) | 24 |
| 嵌入式 C 语言自我修养 | 从编译链接到内核素养 | [html](./01-c-language/05-Kernel-Prep-Embedded-C-Self-Cultivation/html/index.html) | 10 |
| C Traps and Pitfalls | Koenig · C 陷阱与缺陷 | [html](./01-c-language/06-Reference-C-Traps-and-Pitfalls/html/index.html) | 11 |

### 数字逻辑 · 体系结构（5 本）

| 书 | 定位 | HTML 入口 | 页数 |
|---|------|-----------|:----:|
| 数字逻辑与 CPU | Digital Design 实践笔记 · RPi | [html](./00-digital-logic-cpu/html/index.html) | 9 |
| Computer Systems | CSAPP · 深入理解计算机系统 | [html](./02-computer-systems/html/index.html) | 13 |
| Computer Architecture | 量化研究方法 · QCA 笔记 | [html](./15-computer-architecture/html/index.html) | 20 |
| AArch64 实践 | ARM64 体系结构与汇编实践 | [html](./07-arm-architecture/aarch64-practice/html/index.html) | 23 |
| ARM32 汇编 | ARM 汇编语言与体系结构 | [html](./07-arm-architecture/arm32-asm/html/index.html) | 22 |

### Linux 内核（4 本）

| 书 | 定位 | HTML 入口 | 页数 |
|---|------|-----------|:----:|
| Linux Kernel Development | Robert Love · LKD 3e 中文笔记 | [html](./05-linux-kernel/html/index.html) | 20 |
| 现代内核特性 | scheduler / RCU / arm64 / PREEMPT_RT | [html](./05.5-modern-kernel/html/index.html) | 12 |
| 内核调试 | printk / kprobes / ftrace / kgdb | [html](./05.6-kernel-debugging/html/index.html) | 12 |
| Linux 内核深度 | Understanding the Linux Kernel · 深入解析 | [html](./16-linux-kernel-deep/html/index.html) | 22 |

### 内存管理（2 本）

| 书 | 定位 | HTML 入口 | 页数 |
|---|------|-----------|:----:|
| Linux 虚拟内存管理 | Mel Gorman ULVM + 附录 A–M 源码导读 | [html](./06-linux-mm/html/index.html) | 27 |
| 现代内存管理 | memblock / slub / maple tree / mglru / DAMON | [html](./06.5-modern-mm/html/index.html) | 10 |

### 系统编程 · 网络 API（5 本）

| 书 | 定位 | HTML 入口 | 页数 |
|---|------|-----------|:----:|
| Linux 用户态 API | TLPI · Linux 系统编程接口 | [html](./03-linux-userspace-api/html/index.html) | 64 |
| UNP · 卷1 基础篇 | Unix 网络编程 · 基本套接字 | [html](./03.5-unix-network-api/1_BasicFoundation/html/index.html) | 8 |
| UNP · 卷1 进阶篇 | Unix 网络编程 · 高级 IO 与线程 | [html](./03.5-unix-network-api/2_AdvancedSkill/html/index.html) | 5 |
| UNP · 卷1 深化篇 | Unix 网络编程 · 原始套接字与广播 | [html](./03.5-unix-network-api/3_DeepMaster/html/index.html) | 8 |
| UNP · 卷1 设计篇 | Unix 网络编程 · SCTP 与架构设计 | [html](./03.5-unix-network-api/4_ArchitectureDesign/html/index.html) | 10 |

### 网络协议 · 内核网络（8 本）

| 书 | 定位 | HTML 入口 | 页数 |
|---|------|-----------|:----:|
| TCP/IP 详解 | 卷一 协议 · 中文笔记 | [html](./11-tcpip-protocols/html/index.html) | 18 |
| Wireshark 抓包分析 | 包分析实践 · 全 13 章 | [html](./11.5-wireshark-packet-analysis/html/index.html) | 15 |
| 抓包 · HFT 场景 | 低延迟 TCP 延迟 / 卸载 / bypass 实战 | [html](./11.5-wireshark-packet-analysis/hft-scenarios/html/index.html) | 1 |
| 抓包 · 速查表 | 安装验证与常用命令笔记 | [html](./11.5-wireshark-packet-analysis/cheatsheet/html/index.html) | 1 |
| Linux 内核网络 | 深入理解 Linux 网络技术内幕 | [html](./12-kernel-networking/html/index.html) | 17 |
| 现代内核网络 | NAPI / XDP / eBPF / io_uring | [html](./12.5-modern-networking/html/index.html) | 15 |
| DPDK 入门 | DPDK 应用基础 · 初阶 | [html](./13-dpdk/01-Intro-Book/html/index.html) | 15 |
| DPDK 进阶 | DPDK 深入与性能调优 | [html](./13-dpdk/02-Advanced-Book/html/index.html) | 1 |

### 性能分析 · eBPF（3 本）

| 书 | 定位 | HTML 入口 | 页数 |
|---|------|-----------|:----:|
| Systems Performance | Brendan Gregg · 企业版中文笔记 | [html](./06.6-systems-performance/html/index.html) | 21 |
| BPF Performance Tools | Brendan Gregg · 上下册笔记 | [html](./06.7-bpf-observability/02-bpf-performance-tools/html/index.html) | 23 |
| Learning eBPF | O'Reilly · 入门到 verifier | [html](./06.7-bpf-observability/01-learning-ebpf/html/index.html) | 11 |

### HFT · 量化 · 运动控制（3 本）

| 书 | 定位 | HTML 入口 | 页数 |
|---|------|-----------|:----:|
| HFT 工程 | 高频交易全栈工程笔记 | [html](./14-hft-engineering/html/index.html) | 13 |
| 市场微观结构 | Harris · Trading and Exchanges | [html](./19-markets-microstructure/html/index.html) | 29 |
| 运动控制 | PID / IMU / 电机 / 飞控调度 | [html](./10-motion-control/html/index.html) | 5 |

### C++（11 本）

| 书 | 定位 | HTML 入口 | 页数 |
|---|------|-----------|:----:|
| C++ Primer | C++ Primer · 入门语法精读 | [html](./04-cpp/M0-entry-syntax/01-C++Primer/html/index.html) | 19 |
| Effective Modern C++ | Scott Meyers · 现代 C++ 42 条款 | [html](./04-cpp/M1-modern-cpp/01-Effective-Modern-C++/html/index.html) | 8 |
| C++ 网络编程 | 套接字 / epoll / 序列化实践 | [html](./04-cpp/M2-cpp-network-programming/html/index.html) | 1 |
| C++ 对象模型 | Inside the C++ Object Model | [html](./04-cpp/M3-deep-principles/01-Cpp-Object-Model/html/index.html) | 8 |
| C++ 并发 | C++ Concurrency in Action | [html](./04-cpp/M3-deep-principles/02-Cpp-Concurrency/html/index.html) | 15 |
| Effective C++ | Meyers · 55 条款 | [html](./04-cpp/M4-engineering-standards/01-Effective-C++/html/index.html) | 9 |
| More Effective C++ | Meyers · 35 条款 | [html](./04-cpp/M4-engineering-standards/02-More-Effective-C++/html/index.html) | 7 |
| Effective STL | Meyers · 50 条款 | [html](./04-cpp/M4-engineering-standards/03-Effective-STL/html/index.html) | 9 |
| STL 源码剖析 | 侯捷 · 容器与算法源码 | [html](./04-cpp/M4-engineering-standards/04-STL-Source-Analysis/html/index.html) | 11 |
| C++17 完全指南 | C++17 the Complete Guide | [html](./04-cpp/M5-advanced-standards/01-C++17-The-Complete-Guide/html/index.html) | 35 |
| C++20 完全指南 | C++20 the Complete Guide | [html](./04-cpp/M5-advanced-standards/02-C++20-The-Complete-Guide/html/index.html) | 24 |

### Rust（8 本）

| 书 | 定位 | HTML 入口 | 页数 |
|---|------|-----------|:----:|
| Rust · The Book | The Rust Programming Language | [html](./17-rust-foundation/00-Book/html/index.html) | 19 |
| Rust · 扩展阅读 | Rust 生态与扩展索引 | [html](./17-rust-foundation/01-ER/html/index.html) | 6 |
| Rust · 引用与生命周期 | Rust for Rustaceans 方向 | [html](./17-rust-foundation/02-RFR/html/index.html) | 13 |
| Rust · 标准库深入 | core / alloc / std 剖析 | [html](./17-rust-foundation/03-DeepRustStdLib/html/index.html) | 14 |
| Rust · Nomicon | 不安全的 Rust 圣经 | [html](./17-rust-foundation/04-Rust-Nomicon/html/index.html) | 10 |
| Rust · 异步与并发 | Async / Atomics / 网络 | [html](./17-rust-foundation/05-Async-Concurrency-Network/html/index.html) | 3 |
| Rust · 编译器与 LLVM | 编译原理与 LLVM 学习 | [html](./17-rust-foundation/06_Compilers-and-LLVM-Learning/html/index.html) | 4 |
| Rust · WebAssembly | Programming WebAssembly with Rust | [html](./17-rust-foundation/07-Programming-WebAssembly-with-Rust/html/index.html) | 8 |

### 嵌入式 · 驱动（4 本）

| 书 | 定位 | HTML 入口 | 页数 |
|---|------|-----------|:----:|
| 嵌入式系统入门 | 系统启动与内核构建概览 | [html](./08-embedded-boot-build/primer-system-overview/html/index.html) | 19 |
| 工具链与 Yocto | 交叉工具链与 Yocto 构建 | [html](./08-embedded-boot-build/build-toolchain-yocto/html/index.html) | 21 |
| Linux 设备驱动 · 经典 | LDD3 · 字符设备与并发 | [html](./09-device-drivers-dt/classic-driver-theory/html/index.html) | 18 |
| Linux 设备驱动 · 现代 | platform / DT / i2c / regmap / IIO | [html](./09-device-drivers-dt/modern-driver-practice/html/index.html) | 22 |

</details>

> 各书 HTML 由 `build_all.py` + `cfs-style.css` 从笔记 md 实时生成；笔记更新后 `python build_all.py` 即可重新构建。

---

## 延展文档

| 文件 | 内容 |
|------|------|
| [READING-LIST.md](./READING-LIST.md) | 9 本核心 + 7 本 HFT 原版 + 外部书目的**章节级**精读/选读/跳过标签 |
| [HFT-READING-ROADMAP.md](./HFT-READING-ROADMAP.md) | 分书小节级指引 · HFT 不漏项检查清单 · 嵌入式支线详情 |
| [14-hft-engineering/HFT-ENGINEERING-LADDER.md](./14-hft-engineering/HFT-ENGINEERING-LADDER.md) | L0–L5 工程阶梯：知识点 / 交付项目 / 硬验收指标 |
| [projects/](./projects/) | P1–P10 项目脚手架 |
