# HFT Low-Latency Practice — 交易系统工程实践

> **原书：** *Developing High-Frequency Trading Systems* — Donadio / Ghosh / Rossier（Packt 2022 · 320 页 · ISBN 9781803242811）
> —— 本模块笔记里的「原书 Ch N」即指此书。
>
> **前置：** `03` TLPI → `05` LKD → `11`–`13` 网络栈
> **动手路线（L0–L5 交付项目 + 验收指标）：** [HFT-ENGINEERING-LADDER.md](./HFT-ENGINEERING-LADDER.md)
> 全链路 → [README.md](../README.md)

---

## 笔记章节 ↔ 原书章节对照

> 本模块目录名按**主题**起，所以笔记编号与原书章号**有几处错位**（下面加粗的是错位项）。
> 原书 TOC 已核对 Packt 官方（2026-09-06）。

| 原书章 | 标题 | 本仓库笔记 | 标签 |
|:------:|------|-----------|:----:|
| Ch 1 | Fundamentals of a HFT System | [ch01](./chapter-01-hft-fundamentals-ecosystem/README.md) | 🟡 历史与全景 |
| Ch 2 | The Critical Components of a Trading System | [ch02](./chapter-02-exchange-architecture-matching/README.md) | **精读** |
| Ch 3 | Understanding the Trading Exchange Dynamics | [ch03](./chapter-03-orderbook-depth-market-data/README.md) | **精读** |
| Ch 4 | HFT System Foundations – From Hardware to OS | [ch04](./chapter-04-hardware-selection-server-config/README.md) | **精读** |
| Ch 5 | Networking in Motion | **[ch06](./chapter-06-low-latency-network-protocol/README.md)** | **精读** |
| Ch 6 §1 | Performance mental model / Context switches | **[ch05](./chapter-05-os-kernel-tuning/README.md)** | **精读** |
| Ch 6 §2–3 | Lock-free structures / Pre-allocate | **[ch07](./chapter-07-lockless-data-structures-memory-layout/README.md)** | **精读** |
| Ch 7 | Logging, Performance, and Networking | **[ch09](./chapter-09-latency-measurement-benchmarking/README.md)** | **精读** |
| Ch 8 | C++ – The Quest for Microsecond Latency | [ch08](./chapter-08-ultra-low-latency-engine-dev/README.md) | **精读** |
| Ch 9 | Java and JVM for Low-Latency Systems | — | 🟡 不做（见下） |
| Ch 10 | Python – Interpreted but Open to High Performance | — | ⚪ 不做（见下） |
| Ch 11 | High-Frequency FPGA and Crypto | **[ch13](./chapter-13-fpga-crypto-hft/README.md)** | 🟡 |

**为什么缺 Ch 9 / Ch 10：** 原书 Part 3 是「用不同语言实现 HFT」——Ch8 C++、Ch9 Java、Ch10 Python。
本仓库主线是 **C++ / Linux 底层**，Java 的 GC/JMH/Disruptor 与 Python 的性能技巧不在路径上，**有意跳过**，不做笔记。

**原书没有、本仓库补的 3 章**（工程落地必需，原书未覆盖）：

| 笔记 | 内容 |
|------|------|
| [ch10](./chapter-10-risk-compliance-slippage/README.md) | 风控合规与滑点 |
| [ch11](./chapter-11-production-deployment-ops/README.md) | 实盘部署运维 |
| [ch12](./chapter-12-market-making-arbitrage/README.md) | 做市与套利 |

---

## 与网络板块的分界

| | `11`–`13` 网络技术栈 | 本模块（`14`） |
|---|--------------------------|-----------------|
| 维度 | TCP/IP、抓包、内核网络、现代网络、DPDK | 交易系统整机工程 |

网络能力从 `11` → `13` 获取；本模块负责**整合落地**。

---

## 从零构建 HFT：路线图

| 步骤 | 内容 | 章节 |
|------|------|------|
| 1 | **架构** Gateway / Book / Strategy / OMS | [Ch1](./chapter-01-hft-fundamentals-ecosystem/README.md) · [Ch8](./chapter-08-ultra-low-latency-engine-dev/README.md) |
| 2 | **硬件/OS** 绑核 · BIOS · Hugepage · Bypass | [Ch4 原理](./chapter-04-hardware-selection-server-config/README.md) · [Ch5 实操](./chapter-05-os-kernel-tuning/README.md) |
| 3 | **IPC** 无锁 Ring · 内存池 | [Ch7（原书 Ch6§2–3）](./chapter-07-lockless-data-structures-memory-layout/README.md) |
| 4 | **语言** C++ 关键路径 | [Ch8（原书 Ch8）](./chapter-08-ultra-low-latency-engine-dev/README.md) · [Ch1 §5 语言选择](./chapter-01-hft-fundamentals-ecosystem/1.5-编程语言选择.md) |
| 5 | **网络** 交换机 · TCP/UDP · 包路径 · PTP | [Ch6（原书 Ch5）](./chapter-06-low-latency-network-protocol/README.md) |
| 6 | **FPGA / Crypto** ns 级 · 云端共址 | [Ch13（原书 Ch11）](./chapter-13-fpga-crypto-hft/README.md) · [Ch4 §4 硬件选型速查](./chapter-04-hardware-selection-server-config/4.4-硬件选型速查.md) |
| 7 | **测量** T2T 分段 · 异步日志 · Bypass 总纲 | [Ch9（原书 Ch7）](./chapter-09-latency-measurement-benchmarking/README.md) |

**入门实操：** [Ch1 实战启动建议](./chapter-01-hft-fundamentals-ecosystem/1.8-实战启动建议.md)

---

## 章节（13 章）

| 章 | 笔记 | 状态 |
|----|------|------|
| 1 | [chapter-01 基础与生态](./chapter-01-hft-fundamentals-ecosystem/README.md) | ✅ 总览 |
| 2 | [chapter-02 关键组件](./chapter-02-exchange-architecture-matching/README.md) | ✅ 要点 |
| 3 | [chapter-03 交易所动态与 LOB](./chapter-03-orderbook-depth-market-data/README.md) | ✅ 要点 |
| 4 | [chapter-04 硬件到 OS](./chapter-04-hardware-selection-server-config/README.md) | ✅ 要点 |
| 5 | [chapter-05 OS 调优 · 上下文切换（原书 Ch6§1）](./chapter-05-os-kernel-tuning/README.md) | ✅ 要点 |
| 6 | [chapter-06 动态网络（原书 Ch5）](./chapter-06-low-latency-network-protocol/README.md) | ✅ 要点 |
| 7 | [chapter-07 无锁与内存池（原书 Ch6§2–3）](./chapter-07-lockless-data-structures-memory-layout/README.md) | ✅ 要点 |
| 8 | [chapter-08 C++ 微秒征途（原书 Ch8）](./chapter-08-ultra-low-latency-engine-dev/README.md) | ✅ 要点 |
| 9 | [chapter-09 日志与 TTT 测量（原书 Ch7）](./chapter-09-latency-measurement-benchmarking/README.md) | ✅ 要点 |
| 10 | [chapter-10 风控合规](./chapter-10-risk-compliance-slippage/README.md) | ✅ 要点 |
| 11 | [chapter-11 实盘运维](./chapter-11-production-deployment-ops/README.md) | ✅ 要点 |
| 12 | [chapter-12 做市与套利（本仓库扩展）](./chapter-12-market-making-arbitrage/README.md) | ✅ 要点 |
| 13 | [chapter-13 FPGA 与 Crypto（原书 Ch11）](./chapter-13-fpga-crypto-hft/README.md) | ✅ 要点 |

---

## HFT 原版专题书目（7 本）

> 与 `05`/`06`/`12` 那些通用系统书（内核 / 网络 / 性能）**互补不重叠**。
> 标签：🔴 必读 · 🟡 选读 · ⚪ 跳过

| 类别 | 书目 | 对本仓库的价值 |
|------|------|---------------|
| 低延迟系统工程 | Donadio《Developing HFT Systems》· Ghosh《Building Low Latency Applications with C++》· Williams《Low-Latency C++ Programming》 | **核心** — 直接对应本模块 |
| 市场微观结构 | Harris《Trading and Exchanges》· Aldridge《HFT》 | 业务地基 |
| 策略数学建模 | Cartea 等《Algorithmic and HFT》 | **暂缓** — 底层开发不前置 |
| 行业纪实 | Lewis《Flash Boys》 | 背景，无代码 |

### ① Developing High-Frequency Trading Systems — Donadio / Ghosh / Rossier

> ⭐ **就是本模块的原书**（Packt 2022 · 320 页 · ISBN 9781803242811）。章节对照见[上文](#笔记章节--原书章节对照)。

**阅读优先级：** L3 网络之后开（需要 `11`–`13` 打底）。

### ② Building Low Latency Applications with C++ — Sourav Ghosh

> Packt 2023 · 506 页 · ISBN 9781837639359 · 12 章 + 配套代码仓库。
> 定位：**从零搭一个完整交易生态**（撮合引擎 + 行情发布 + 订单网关 + 策略）→ **L5 项目参考**。
> 目录已核对 Packt 官方 TOC（2026-09-06）。

| 章 | 标题 | 标签 | HFT 关联 |
|----|------|------|---------|
| Ch 1 | Introducing Low Latency App Development | 🟡 | 概念铺垫，快读 |
| Ch 2 | Designing Common Low Latency Applications | ⚪ | 视频/游戏/IoT —— **跳过**，直奔 Ch3 |
| Ch 3 | C++ Concepts from a Low-Latency Perspective | **精读** | 哪些 C++ 特性该避、编译器优化参数 |
| Ch 4 | **Building Blocks**：内存池 / 无锁队列 / 低延迟日志 / socket | **精读** | 与 [ch07](./chapter-07-lockless-data-structures-memory-layout/README.md) 直接对应，四件套 |
| Ch 5 | Designing Our Trading Ecosystem | **精读** | 生态布局总图 |
| Ch 6 | **Building the C++ Matching Engine** | **精读** | LOB + 撮合 —— 做 P8 时逐节对照 |
| Ch 7 | Communicating with Market Participants | **精读** | 自定义行情/订单协议、订单网关、行情发布 |
| Ch 8 | Processing Market Data and Sending Orders | **精读** | 组播订阅 + 解码 + 重建 LOB |
| Ch 9 | Trading Algorithm Building Blocks | 🟡 | 仓位/PnL/风控 —— 业务侧 |
| Ch 10 | Market Making and Liquidity Taking | 🟡 | 做市与吃单 —— 业务侧 |
| Ch 11 | **Adding Instrumentation and Measuring Performance** | **精读** | 埋点 + 分段延迟 —— 与 [ch09](./chapter-09-latency-measurement-benchmarking/README.md) 互补 |
| Ch 12 | Analyzing and Optimizing Performance | **精读** | 优化技巧收尾 |

### ③ Low-Latency C++ Programming — Antony Williams

> ⚠️ **目录未核对**（未检索到官方 TOC），下表按**主题**给，不按章号。
> 作者 = *C++ Concurrency in Action* 作者（本仓库 [04/M3 C++ 并发](../04-cpp/M3-deep-principles/02-Cpp-Concurrency) 同一人）。

| 主题 | 标签 | 嵌入级别 |
|------|------|---------|
| CPU cache / cache line / 伪共享 | **精读** | **L4** — 与 Hennessy Ch2 交叉 |
| NUMA 与内存布局 | **精读** | L4 |
| 内存序与屏障（acquire/release/seq_cst） | **精读** | L4 — **与 ARM64 弱序对照** |
| 无锁队列 | **精读** | L4 — 与 [ch07](./chapter-07-lockless-data-structures-memory-layout/README.md) 交叉 |
| 锁的开销与确定性行为 | **精读** | L4 |
| 微秒级服务的测量方法 | **精读** | L5 |

### ④ High-Frequency Trading — Irene Aldridge

> 有中译本。**⚠️ 注意：国内流传的《高频交易》中文版与 Aldridge 原书内容差异需自行甄别。**

| 主题 | 标签 | 说明 |
|------|------|------|
| HFT 策略分类（做市 / 统计套利 / 延迟套利） | 🟡 | **业务视角**，理解上层在干什么 |
| 风险框架 | 🟡 | 上实盘前补 |
| 微观结构实证 | 🟡 | 与 Harris 对照 |

> 定位：**L5 之后再补**。做底层网关/网络层时它不阻塞。

### ⑤ Algorithmic and High-Frequency Trading — Cartea / Jaimungal / Penalva

> 剑桥大学出版社。**暂缓**。

| 主题 | 标签 | 说明 |
|------|------|------|
| 随机过程、最优执行 | ⚪ | 目标底层开发 → **不前置** |
| 做市商模型（Avellaneda-Stoikov 一类） | ⚪ | 转策略研究时再开 |
| 订单簿动态建模 | 🟡 | 唯一可提前看的部分 |

### ⑥ Flash Boys — Michael Lewis

> 行业纪实，**无代码**。理解 HFT 行业生态与历史（光纤直连、IEX、暗池）。任意空隙读。

### 阅读优先级（对应当前位置）

| 梯队 | 书目 | 触发条件 |
|------|------|---------|
| **一** | Harris（见 [`19`](../19-markets-microstructure/)）· ③ Williams | **现在就能开** — 一个管业务、一个管底层 C++ |
| **二** | ② Ghosh | 开始做三进程交易链路（L5 项目）时按章对照 |
| **三** | ① Donadio（架构全景）· ④ Aldridge（业务） | L3 网络之后 |
| 后置 | ⑤ Cartea（数学） | 除非转向策略研究 |
| 消遣 | ⑥ Flash Boys | 随时 |

---

## 交叉阅读

- **动手路线（L0–L5 工程阶梯 + 验收指标）** → [HFT-ENGINEERING-LADDER.md](./HFT-ENGINEERING-LADDER.md)
- [03-linux-userspace-api](../03-linux-userspace-api/) · [04/M2 C++ 网络编程](../04-cpp/M2-cpp-network-programming/)
- [13-DPDK](../13-dpdk/) · [19-markets-microstructure](../19-markets-microstructure/)
- [18-Rust 量化](../18-rust-quant/) · [projects/P8 撮合引擎](../projects/P8-matching-engine/) · [projects/P10 HFT 原型](../projects/P10-hft-prototype/)
