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

### 1.1 热路径与冷路径（Tick-to-Trade 生命线）

**热路径（hot path）**：从行情包进入到订单发出，这条生死攸关的关键代码链路：

```
行情包接收 → 解析报文 → 更新盘口 → 策略信号计算 → 风控校验 → 发送下单
```

**HFT 的热路径和普通软件定义不同**：

| | 普通软件 | HFT |
|---|---------|-----|
| 热路径判据 | 执行**次数最多**的代码 | 关键链路上的代码，**哪怕执行次数少** |
| 衡量指标 | 吞吐量（ops/s） | **尾延迟**（p99/p999），输不起 |
| 特点 | 均匀负载 | 大多数 tick 不触发下单；**一旦触发不能卡顿** |

**冷路径（cold path）**：一切不准进热路径的慢活，全部通过**无锁队列**扔给冷线程处理，热路径只跑关键逻辑、绝不等待：

- 日志打印、落盘、回写数据库
- `malloc` 等动态内存分配
- 统计指标、仪表盘上报

### 热路径工程硬性约束

| 约束 | 原因 | 手段 |
|------|------|------|
| **禁系统调用** | 用户/内核上下文切换带来几十~上百 μs 抖动 | `send`/`recv` 用 DPDK/OpenOnload 用户态网卡绕开内核网络栈 |
| **禁动态分配** | `malloc` 延迟不定、可能触发 `brk`/`mmap` 系统调用 | 启动期一次性建**内存池**，运行期只从池里取（→ [Ch8 §5](./chapter-08-超低延迟核心引擎开发.md)） |
| **少分支判断** | 分支预测失败 → 流水线清空，延迟暴涨 | 查表、位运算代替 if-else；`__builtin_expect` / `likely`/`unlikely` 提示 |
| **保 Cache 命中** | Cache miss 访问主存 ~100ns，是 L1 的几十倍 | 结构体布局紧凑、`alignas` 避免伪共享、数据/代码**预热**进 L1/L2 |
| **独占 CPU 核** | 内核调度、其他进程打扰 | `taskset`/`sched_setaffinity` 绑核 + `isolcpus` `nohz_full` 内核隔离（→ [Ch5](./chapter-05-操作系统内核极致调优.md)） |

**类比**：热路径是短跑接力选手——接力棒（行情包）一到手必须立刻冲出去下单；喝水、记笔记、统计成绩这些杂事（日志/统计）全交场边后勤（冷路径线程），绝不能让选手停下来干杂活。

**观测注意**：eBPF 可定位热路径延迟尖峰（Cache-miss、内核抢占），但 kprobe/uprobe 本身有开销，**生产热路径上慎用**；日常用 perf 硬件计数器（`cache-misses`、`branch-misses`）低开销采样即可（→ [06.7-bpf-observability](../06.7-bpf-observability/README.md)）。

<details>
<summary>Quiz：为什么 HFT 热路径严禁调用 printf 打印日志？</summary>

三层代价，层层致命：

1. **格式化开销**：`printf` 要解析格式串、逐参转字符串——纯用户态 CPU 消耗，几十 ns~μs 级
2. **write() 系统调用**：真正输出触发 syscall → 用户/内核上下文切换，几十~上百 μs 抖动，还可能阻塞（终端慢、缓冲满）
3. **锁与串行化**：stdout 是全局锁，多线程争用引入不确定等待；glibc 缓冲策略（行缓冲/全缓冲）让行为更不可预测

正解：热路径只把**预序列化的二进制记录**（定长、无格式化）写进 SPSC 无锁 ring，冷线程消费 ring 落盘——热路径开销压到一次内存写。

</details>

---

## 2. 硬件与操作系统优化

| 手段 | 目的 |
|------|------|
| **CPU Pinning / `isolcpus`** | 热点线程独占核，**消除上下文切换** |
| **BIOS：关 HT / C-states / Turbo** | 降低 **jitter** |
| **Kernel Bypass**（Solarflare + OpenOnload 等） | 用户态轮询 NIC；**零拷贝**；UDP/TCP **1.5–10 μs → 0.5–2 μs** |
| **Memory Pool** | 热点路径**禁止** `malloc`/`new` |
| **Huge Pages** | 减少 **TLB miss** |

→ 深化：[chapter-04 硬件](./chapter-04-硬件选型与服务器配置.md) · [chapter-05 OS 调优](./chapter-05-操作系统内核极致调优.md) · [13-DPDK](../13-dpdk/)

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

→ 深化：[chapter-06 网络与协议](./chapter-06-低延迟网络与协议优化.md) · [19-markets-microstructure](../19-markets-microstructure/)

---

## 6. FPGA（纳秒级）

当 **1–5 μs 软件**仍不够：将 **MD 解析、协议栈、简单执行** 烧进 **FPGA**。

| 特点 | 说明 |
|------|------|
| **无 OS / 调度 / 中断** | 并行硬件，**确定性** |
| **T2T** | 可压至 **<500 ns** |

→ 深化：[chapter-15 FPGA 与 Crypto（原书 Ch11）](./chapter-15-fpga-与加密货币高频.md) · [chapter-04 §4](./chapter-04-硬件选型与服务器配置.md#4-硬件选型速查工程)

---

## 实战启动建议

1. **Linux 多进程** — 每关键进程 **绑核**（`taskset` / `isolcpus`）
2. **C++ 极简 LOB** — 单品种、固定深度
3. **无锁共享内存 Ring** — 进程间行情/订单（`mmap` + cache line 对齐）
4. **Solarflare + OpenOnload**（或 **DPDK**）— 跑通 **Gateway IN → Book → Strategy** 数据流
5. **先量后优** — 全链路 **硬件时间戳 + PTP**；记录 **p50/p99/p999 T2T**

→ [chapter-10 延迟测量](./chapter-10-延迟测量与基准压测.md) · [chapter-12 实盘运维](./chapter-12-实盘上线与运维进阶.md)

---

## 章节路线图（Ch2–15）

> **原书 Ch1–11** 已全部映射；**Ch11–13** 为本仓库工程扩展（风控 / 运维 / 策略）。

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
| FPGA / Crypto | Ch15（原书 Ch11） |
| T2T 基准 / Jitter / 日志 | Ch10（原书 Ch7） |
| 风控 / 合规 / 滑点 | Ch11（本仓库扩展） |
| 上线 / 监控 / 运维 | Ch12 |
| 做市 / 套利策略 | Ch13（本仓库扩展） |

---

## 交叉阅读

| 仓库 | 对照 |
|------|------|
| [13-DPDK](../13-dpdk/) | 用户态网卡 · PMD · 零拷贝 |
| [03-linux-userspace-api](../03-linux-userspace-api/) | `mmap` · 进程 · 定时 |
| [projects/P9-os-from-scratch](../projects/P9-os-from-scratch/) | OS/内存/中断体感 |
| [19-markets-microstructure](../19-markets-microstructure/) | 微观结构 · 订单簿理论 |
