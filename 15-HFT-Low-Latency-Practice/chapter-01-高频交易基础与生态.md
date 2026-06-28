# 第1章 高频交易基础与生态

> **从零构建 HFT 的总览** · Tick-to-Trade · 关键路径 · 语言栈 · 实战起步

---

## 本章定位

构建从零开始的高频交易系统，是**底层硬件、操作系统、网络协议与高级软件工程**的跨学科工程。核心指标是 **Tick-to-Trade（T2T）**：从接收行情到发出订单的端到端延迟，目标通常在**微秒（μs）甚至纳秒（ns）**量级。

| 维度 | HFT 与普通交易系统 |
|------|-------------------|
| **延迟** |  μs/ns 级；拼**平均延迟**更拼**最大延迟（tail latency）** |
| **确定性** | 禁用 Turbo、HT、节能等**非确定性**特性 |
| **热点路径** | 无 `malloc`、无锁、无异常、无内核拷贝 |
| **测量** | 一切优化必须建立在**精确 T2T 测量**之上 |

---

## 1. 系统核心架构（关键路径）

完整 HFT 系统至少包含以下组件：

```
Exchange ──► Gateway IN ──► Book Builder ──► Strategy ──► OMS ──► Gateway OUT ──► Exchange
                 │              │                │           │
              行情 UDP/TCP    本地 LOB        Signal+Exec   风控/合规
```

| 组件 | 职责 |
|------|------|
| **Gateway IN** | 连接交易所，接收 **market data**（ITCH/FAST/SBE 等） |
| **Book Builder** | 维护本地 **Limit Order Book**；更新尽量 **O(1)**，策略可瞬时读 BBO |
| **Strategy** | **Signal**（何时）+ **Execution**（如何下单）；系统「大脑」 |
| **Order Manager (OMS)** | 订单生命周期；**内部风控**（超限直接拒单，不等交易所拒绝） |
| **Gateway OUT** | 订单发往交易所（OUCH 等）；与 IN 常分离进程/线程 |

→ 深化：[chapter-02 关键组件](./chapter-02-交易所架构与撮合原理.md) · [chapter-08 核心引擎](./chapter-08-超低延迟核心引擎开发.md) · [chapter-03 订单簿](./chapter-03-订单簿深度与行情解析.md)

---

## 2. 硬件与操作系统优化

| 手段 | 目的 |
|------|------|
| **CPU Pinning / `isolcpus`** | 热点线程独占核，**消除上下文切换** |
| **BIOS：关 HT / C-states / Turbo** | 降低 **jitter** |
| **Kernel Bypass**（Solarflare + OpenOnload 等） | 用户态轮询 NIC；**零拷贝**；UDP/TCP **1.5–10 μs → 0.5–2 μs** |
| **Memory Pool** | 热点路径**禁止** `malloc`/`new` |
| **Huge Pages** | 减少 **TLB miss** |

→ 深化：[chapter-04 硬件](./chapter-04-硬件选型与服务器配置.md) · [chapter-05 OS 调优](./chapter-05-操作系统内核极致调优.md) · [13-DPDK](../14-DPDK-Low-Latency-Network/)

---

## 3. 无锁数据结构与 IPC

| 问题 | 方案 |
|------|------|
| **锁** → 阻塞、死锁、上下文切换 | **Lock-free Ring Buffer**（LMAX Disruptor 思路） |
| **缓存** | 连续内存环，提高 **cache locality** |
| **用途** | 行情分发、日志、策略↔OMS 队列 |

→ 深化：[chapter-07 无锁与内存布局](./chapter-07-无锁数据结构与内存布局.md)

---

## 4. 编程语言选择

### C++（关键路径首选）

| 原则 | 说明 |
|------|------|
| **模板** | 编译期多态，便于 **inline** |
| **避免虚函数** | vtable + 分支预测失败；可用 **CRTP** |
| **内存序** | `memory_order_acquire/release`；避免默认 **seq_cst** |
| **禁止热点异常** | 抛异常 **数千周期** |

### Java

| 原则 | 说明 |
|------|------|
| **GC** | STW 致命；ZGC/Shenandoah/Epsilon · **零对象创建** |
| **Autoboxing** | 避免；**对象池** + **primitive** |
| **JVM 预热** | JMH · 假订单；Azul **ReadyNow** · Graal AOT |
| **Disruptor** | 无锁环 IPC · Mechanical Sympathy |

→ 深化：[chapter-09 Java/JVM（原书 Ch9）](./chapter-09-java-jvm-低延迟系统.md)

### Python

| 角色 | **研究、回测、编排** — 非 μs 执行路径 |
|------|--------------------------------------|
| **瓶颈** | 解释 · **GIL** · 无高效 JIT |
| **生产** | **C++ 核心 `.so`** — Boost.Python / Cython / SWIG |

→ 深化：[chapter-14 Python 混合架构（原书 Ch10）](./chapter-14-python-高性能混合架构.md) · [chapter-08 C++ 引擎](./chapter-08-超低延迟核心引擎开发.md)

---

## 5. 网络协议与物理传输

| 层 | 选择 |
|----|------|
| **内外网** | 弃 FIX 文本 → **FAST / ITCH / OUCH / CME SBE** 等二进制 |
| **跨机房** | 芝加哥↔纽约：**微波 / 空芯光纤**（空气中光速比玻璃快 ~50%）→ **latency arbitrage** |

→ 深化：[chapter-06 网络与协议](./chapter-06-低延迟网络与协议优化.md) · [00-Trading-and-Exchanges](../00-Trading-and-Exchanges/)

---

## 6. FPGA（纳秒级）

当 **1–5 μs 软件**仍不够：将 **MD 解析、协议栈、简单执行** 烧进 **FPGA**。

| 特点 | 说明 |
|------|------|
| **无 OS / 调度 / 中断** | 并行硬件，**确定性** |
| **T2T** | 可压至 **<500 ns** |

→ 深化：[chapter-04 硬件](./chapter-04-硬件选型与服务器配置.md)

---

## 实战启动建议

1. **Linux 多进程** — 每关键进程 **绑核**（`taskset` / `isolcpus`）
2. **C++ 极简 LOB** — 单品种、固定深度
3. **无锁共享内存 Ring** — 进程间行情/订单（`mmap` + cache line 对齐）
4. **Solarflare + OpenOnload**（或 **DPDK**）— 跑通 **Gateway IN → Book → Strategy** 数据流
5. **先量后优** — 全链路 **硬件时间戳 + PTP**；记录 **p50/p99/p999 T2T**

→ [chapter-10 延迟测量](./chapter-10-延迟测量与基准压测.md) · [chapter-12 实盘运维](./chapter-12-实盘上线与运维进阶.md)

---

## 章节路线图（Ch2–12）

| 主题 | 章节 |
|------|------|
| 交易所 / 关键组件 / Gateway | Ch2 |
| LOB / 撮合规则 / 行情解析 | Ch3 |
| 服务器 / BIOS / FPGA | Ch4 |
| Linux 内核 / 绑核 / Hugepage | Ch5 |
| 二进制协议 / 物理链路 | Ch6 |
| Disruptor / 内存布局 | Ch7 |
| Gateway·Strategy·OMS 实现 | Ch8 |
| Java/JVM 低延迟 | Ch9（原书） |
| Python 混合架构 | Ch14（原书 Ch10） |
| T2T 基准 / Jitter / 日志 | Ch10（原书 Ch7） |
| 风控 / 合规 / 滑点 | Ch11 |
| 上线 / 监控 / 运维 | Ch12 |
| 做市 / 套利策略 | Ch13（本仓库扩展） |

---

## 交叉阅读

| 仓库 | 对照 |
|------|------|
| [14-DPDK](../14-DPDK-Low-Latency-Network/) | 用户态网卡 · PMD · 零拷贝 |
| [08-TLPI](../08-The-Linux-Programming-Interface/) | `mmap` · 进程 · 定时 |
| [09-system-low-level-hands-on](../09-system-low-level-hands-on/) | OS/内存/中断体感 |
| [00-Trading-and-Exchanges](../00-Trading-and-Exchanges/) | 微观结构 · 订单簿理论 |
