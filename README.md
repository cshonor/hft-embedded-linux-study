# hft-embedded-linux-study

> **HFT 低延迟 Linux 底层** + **嵌入式 Linux 飞控** 双线笔记仓库。
> [github.com/cshonor/hft-embedded-linux-study](https://github.com/cshonor/hft-embedded-linux-study) ·
> **HTML 阅读版（59 本，离线可读）** → [html/index.html](./html/index.html) ·
> [GitHub Pages](https://cshonor.github.io/hft-embedded-linux-study/)

```bash
git clone https://github.com/cshonor/hft-embedded-linux-study.git
```

---

## 模块总览（编号 = 学习顺序）

> **递进主轴：** 硬件底层 → 编程语言 → Linux 系统 → 驱动/设备树 → 嵌入式工程 → 网络栈 → 性能工具 → HFT 上层业务
> `.5` / `.6` / `.7` 不是子模块，是**现代补丁层**——经典书基于 2.6/3.x 内核，这些模块补齐 5.x/6.x 的真实实现。

| # | 文件夹 | 定位 | Phase |
|---|--------|------|:-----:|
| **00** | [digital-logic-cpu](./00-digital-logic-cpu/) | 硬件底层：组合/时序/CPU 词汇 | **1** |
| **01** | [c-language](./01-c-language/) | C / 指针 / GNU-C | 2 |
| **02** | [computer-systems](./02-computer-systems/) | 程序=机器：栈/缓存/VM/并发 | 2 |
| **03** | [linux-userspace-api](./03-linux-userspace-api/) | 用户态系统编程（TLPI） | 3 |
| **03.5** | [unix-network-api](./03.5-unix-network-api/) | Socket API 精读（UNP） | 3 |
| **03.6** | [userspace-debugging](./03.6-userspace-debugging/) | gdb / strace / valgrind / ASan / TSan | 3 |
| **04** | [cpp](./04-cpp/) | Modern C++ / 并发 / 对象模型 / muduo 网络 | 3 穿插 |
| **05** | [linux-kernel](./05-linux-kernel/) | 内核入门（LKD） | 4 |
| **05.5** | [modern-kernel](./05.5-modern-kernel/) | 现代内核**非 MM**：调度/RCU/ARM64 | 4 |
| **05.6** | [kernel-debugging](./05.6-kernel-debugging/) | KASAN / KGDB / Ftrace | 4 |
| **06** | [linux-mm](./06-linux-mm/) | 内核内存管理（Gorman） | 4 |
| **06.5** | [modern-mm](./06.5-modern-mm/) | 现代 MM：memblock/SLUB/MGLRU/DAMON | 4 |
| **07** | [arm-architecture](./07-arm-architecture/) | ARM / AArch64 | 5A |
| **08** | [embedded-boot-build](./08-embedded-boot-build/) | U-Boot / 内核构建 / rootfs | 5A |
| **09** | [device-drivers-dt](./09-device-drivers-dt/) | 驱动 + 设备树 | 5A |
| **10** | [motion-control](./10-motion-control/) | PID / 姿态 / 飞控（兴趣） | 5A/6 |
| **11** | [tcpip-protocols](./11-tcpip-protocols/) | TCP/IP 协议（Stevens 卷一） | 5B |
| **11.5** | [wireshark-packet-analysis](./11.5-wireshark-packet-analysis/) | 抓包分析实战 | 5B |
| **12** | [kernel-networking](./12-kernel-networking/) | 内核网络栈（Rosen） | 5B |
| **12.5** | [modern-networking](./12.5-modern-networking/) | 现代网络：XDP / eBPF / io_uring | 5B |
| **13** | [dpdk](./13-dpdk/) | 用户态高速网络 | 5B |
| **06.6** | [systems-performance](./06.6-systems-performance/) | 系统性能方法论（Gregg） | 5B |
| **06.7** | [bpf-observability](./06.7-bpf-observability/) | BPF / 可观测（Gregg） | 5B |
| **14** | [hft-engineering](./14-hft-engineering/) | HFT 工程实践 | 5B |
| **15** | [computer-architecture](./15-computer-architecture/) | 体系结构加深（拓展） | 6 |
| **16** | [linux-kernel-deep](./16-linux-kernel-deep/) | 内核深度 ULK3（拓展） | 6 |
| **17** | [rust-foundation](./17-rust-foundation/) | Rust 基础（拓展） | 6 |
| **18** | [rust-quant](./18-rust-quant/) | Rust 量化（拓展） | 6 |
| **19** | [markets-microstructure](./19-markets-microstructure/) | 交易 / 市场微观结构 | 6 |

---

## 学习路线

```
Phase1  00 数字逻辑/CPU
   ↓
Phase2  01 C → 02 计算机系统
   ↓
Phase3  03 用户态 API → 03.5 socket → 03.6 调试 → 穿插 04 C++
   ↓
Phase4  05 内核 → 05.5 现代内核 → 05.6 调试 → 06 MM → 06.5 现代 MM
   ↓
Phase5  A 嵌入式: 07 → 08 → 09 → P5 板级实战
        B HFT:    11 → 11.5 → 12 → 12.5 → 13 → 06.6 → 06.7 → 14
   ↓
Phase6  拓展: 15 · 16 · 17 · 18 · 19
```

**必须警惕：**

1. **禁止乱跳** —— Phase1/2 没过不要冲内核、DPDK、HFT。
2. **时间不均分** —— 电机、Rust 量化、体系结构加深前期少投入。
3. **必须动手** —— 无锁队列、绑核、大页、简易 UDP；嵌入式侧编译内核、调设备树。
4. **别拿旧书 API 对照 6.x 源码** —— bootmem→memblock、SLAB→SLUB、highmem 在 ARM64 不存在、LRU→MGLRU、page→folio。要查现代实现去 `.5` / `.6` 模块。

---

## 动手线

**不是先读完书再做项目，项目本身就是路径**——卡住了翻书查对应模块。

### 能力阶梯 L0 → L5

> 完整版（每级知识点表 + 交付细节 + 硬验收指标）→ [14-hft-engineering/HFT-ENGINEERING-LADDER.md](./14-hft-engineering/HFT-ENGINEERING-LADDER.md)
> **当前位置：L0**（《Pointers on C》§7.1 stream model / FILE 对象，与 L0 的「自实现 `malloc`」正好咬合）

| 级 | 核心知识 | 交付项目 | 硬验收指标 |
|:--:|----------|----------|------------|
| **L0** | 指针算术、struct 布局、堆分配、ABI | 自实现 `malloc` + 对齐/合并 benchmark | 能解释 chunk header / bins / `M_MMAP_THRESHOLD` |
| **L1** | TLPI：fd、线程、`mmap`、信号、`epoll` | 多线程 TCP echo server（epoll ET + 线程池） | p99 < 200μs；能画出请求完整路径 |
| **L2** | LKD + Gorman：调度/中断/VMA/页表/slab | `perf` 定位并消除一次真实抖动 | 能用火焰图 + `perf stat` 说清瓶颈归属 |
| **L3** | 组播、UDP、socket 选项、NAPI | UDP 组播行情接收器（含丢包统计） | 10 万 pps 下**零丢包**，能说出丢包在哪一层 |
| **L4** | cache line / NUMA / 内存序 / 无锁 | SPSC 无锁 ring（padding 前后对比） | 单跳 < 100ns，p99 < 200ns，**批量消费**版更快 |
| **L5** | DPDK/AF_XDP、LOB、PTP、T2T 测量 | 三进程：FeedHandler → Book → Strategy | 软件栈 tick-to-trade **p99 < 10μs**（自测环境如实记录） |

> **ARM64 汇编在 L4 第一次变现：** x86 是 TSO 强序，`acquire/release` 编译成零指令——「忘了写 `memory_order`」在 x86 上常常碰巧能跑；ARM64 弱序，`ldar`/`stlr` 少一条就直接崩。

### 项目 P1 → P10

| Project | 做什么 | 覆盖模块 | 前置 |
|:-------:|--------|:--------:|:----:|
| **P1** | Logisim 搭 8-bit CPU（ALU+寄存器+FSM） | `00` | 无 |
| **P2** | mini shell（fork/exec/pipe）+ 自制 malloc/free | `01` `02` | P1 |
| **P2.5** | GNU C 工具箱：container_of + 侵入式链表 + 无锁 ring + vtable | `01` | P2 |
| **P3** | 并发 HTTP Server：C 版（epoll+线程池）→ C++ 重写 | `03` `04` | P2 |
| **P3.5** | BusyBox 极简 Linux：内核编译 + rootfs + QEMU 起到 shell | `05` `08` | P3 |
| **P4** | 可加载内核模块：字符设备 + kmalloc 追踪 + /proc 统计 | `05` `05.5` `05.6` `06` | P3+P3.5+P2.5 |
| **P5** | 树莓派嵌入式全链路（5 子项目） | `07`–`10` | P4 |
| **P6** | raw socket 抓包 + 逐层解析 + TCP 流重组 + eBPF 追踪 NAPI | `11` `12` `12.5` `06.7` | P3 |
| **P7** | DPDK packet forwarder + perf 火焰图 + bpftrace 延迟探针 | `13` `06.6` `06.7` | P6 |
| **P8** | 限价订单簿撮合引擎：无锁 ring + 绑核/Hugepage | `14` `18` `19` | P4+P5+P7 |
| **P10** | HFT 单机原型：DPDK 行情 + 撮合 + 策略 + 风控 + 回测 | `13` `14` `15` `19` | P7+P8 |

脚手架 → [`projects/`](./projects/)

---

## 当前状态

- **正在：** `03` TLPI 逐章精读（主线，64 章推进中）· `05` LKD 薄笔记扩写（剩 Ch10 同步方法 8 篇 + 散落 7 篇）· `09` Madieu 驱动 Ch12 DMA（12.4/12.5 待写）· `06.7` eBPF 双书 · `01` Pointers on C（Ch7）
- **下一站：** Madieu 12.4 / 12.5 · LKD Ch10 收官 · TLPI 后续章节
- **板卡清单（Pi5）：** [RASPBERRY-PI5-LABS.md](./projects/P5-raspberry-pi-embedded/RASPBERRY-PI5-LABS.md)

---

## 相关仓库与文档

| | 用途 |
|---|------|
| [cpp-learning-notes](https://github.com/cshonor/cpp-learning-notes) | C / C++ 详细笔记与代码（`01` · `04`） |
| [Computer-Networking](https://github.com/cshonor/Computer-Networking) | Socket / UNP / TCP/IP 实战代码（`03.5` · `04/M2`） |
| [READING-LIST.md](./READING-LIST.md) | **某本书具体读哪几章**（精读 / 选读 / 跳过标签） |
| [HFT-READING-ROADMAP.md](./HFT-READING-ROADMAP.md) | 分书小节指引 · HFT 不漏项检查清单 · 嵌入式支线 |
| [14-hft-engineering/HFT-ENGINEERING-LADDER.md](./14-hft-engineering/HFT-ENGINEERING-LADDER.md) | L0–L5 每级交付项目与硬验收指标 |
