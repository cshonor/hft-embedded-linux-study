## 1.1–1.3 系统性能、角色与活动

### 系统性能是什么

研究对象是**整个计算机系统**在数据路径上的**全部主要软硬件组件**，而非单一进程或单块网卡。

| 典型目标 | 含义 | HFT 对应 |
|----------|------|----------|
| **降低延迟（Latency）** | 缩短等待时间，改善体验 | 行情→决策→发单全链路 μs 级 |
| **提升吞吐量（Throughput）** | 单位时间处理更多工作 | 每秒处理更多 tick/订单 |
| **消除低效、降成本** | 同样硬件做更多事 | 裸机共置、CPU/NUMA 不浪费 |

### 参与角色

| 角色 | 关注点 |
|------|--------|
| 系统管理员 / SRE | 生产稳定性、资源、监控 |
| 应用开发者 | 业务逻辑、工作负载特征 |
| 网络 / DB 管理员 | 专项子系统 |
| **性能工程师** | 深入排查、开发/使用性能工具 |

**HFT：** 小团队常一人兼多角 — 既要懂策略（ workload ），又要懂绑核/网络/内核（ resource ）。

### 生命周期中的 11 项活动（摘要）

设定性能目标 → 原型测试 → 版本基准 → 生产调优 → 监控 → 事故回顾 ……

**HFT 要点：**

- **上线前** 就要有延迟/抖动 SLO（目标），不是上线后才量
- **生产调优** 与 **事故回顾** 对应 tail latency、P99 尖刺排查
- 与 [10-HFT ch10 延迟测量](../../../15-HFT-Low-Latency-Practice/chapter-10-延迟测量与基准压测/) 衔接

---

### HFT · 行情 tick 全链路 Checklist

> **用法：** 先 **workload**（每段在干什么）→ 再 **resource**（哪段慢/抖）→ 对照下面打勾。单点 CPU 跑分不能代替这张表。

**端到端数据路径（单 tick / 单订单）：**

```
[网卡 DMA] → [驱动/NAPI] → [内核协议栈 或 bypass]
      → [copy 到用户态 / mmap ring]
      → [解码：SBE/FIX/私有二进制]
      → [策略：信号 + 风控]
      → [订单序列化 + 发单]
      → [内核/网关 → 网卡 TX]
```

#### A. 工作负载分析 — 「程序在干什么？」

| 段 | 要问的问题 | 常见 hot path |
|----|------------|---------------|
| **收包** | 组播还是 TCP？几路 feed？每 tick 多少字节？ | 每包一次 epoll/read；batch 还是 per-packet |
| **进用户态** | copy 还是 zero-copy（mmap/DPDK/onload）？ | `recvmsg` + 拷贝 vs ring buffer |
| **解码** | 定长还是变长？版本分支？endian | 按 offset 解析；避免 heap/printf |
| **策略** | 热路径算什么？分支可预测吗？ | 均线/价差/库存；`branch-misses` |
| **风控** | 同步还是预检查？锁吗？ | 全局 map vs 单 writer |
| **发单** | 同步 write 还是队列 + 专用线程？ | TCP_NODELAY；connect 是否长连 |

#### B. 资源分析 — 「哪段在等？」

| 段 | 看啥 | 工具 / 章节 |
|----|------|-------------|
| **网卡 → 内核** | softirq、丢包、`rx_missed_errors` | `ethtool -S` · [Ch 10 网络](../chapter-10-network/) |
| **内核栈** | 协议栈延迟、锁、cross-NUMA | `perf`、`ftrace` · [10-DPDK bypass](../../../14-DPDK-Low-Latency-Network/) |
| **CPU** | IPC、frontend/backend stall、绑核是否生效 | `perf stat` · [Ch 6 CPU](../chapter-06-cpus/) |
| **内存** | cache miss、page fault、false sharing | `perf mem` · [Ch 7 内存](../chapter-07-memory/) · [CSAPP Ch6](../../../01-CSAPP-3rd/chapter-06-memory-hierarchy/) |
| **磁盘** | 热路径是否误触日志/sync | 热路径 **不应** 有 disk I/O；NVMe 仅冷路径 |
| **端到端** | P50/P99/P999、抖动 tail | 硬件 timestamp / [HFT ch10 延迟测量](../../../15-HFT-Low-Latency-Practice/chapter-10-延迟测量与基准压测/) |

#### C. 自检清单（上线 / 排障时逐项过）

**架构 & 目标**

- [ ] 写清 **SLO**：tick→决策、决策→发单、端到端 P99 上限（μs/ms）
- [ ] 能画出 **单 tick 路径图**（与上面框图一致，标你的实际组件名）
- [ ] 区分 **测量点**：网卡硬件时间 vs 用户态 `clock_gettime`（含时钟源与开销）

**收包 & 网络**

- [ ] 数据面网卡 **NUMA 与 CPU 同节点**；IRQ/RSS 与收包线程同核或明确 affinitize
- [ ] 知悉走 **内核栈还是 bypass**（DPDK/onload/socket+epoll）及 tradeoff
- [ ] 无静默丢包（`drop`、`no_buffer` 监控）

**进进程 & 解码**

- [ ] 热路径 **无 malloc、无 stdio、无 JSON**（除非 cold path）
- [ ] 协议解析 **layout 与 spec 一致**（→ CSAPP「比特+上下文」）
- [ ] 解码 buffer **预分配 / 环形复用**

**策略 & CPU**

- [ ] 热路径 **绑核**（`isolcpus` / `taskset` / `pthread_setaffinity`）
- [ ] Release 构建：**`-O3 -march=native -DNDEBUG`**，CI 与生产一致
- [ ] 看过 hot path **汇编或 perf 火焰图**，知悉 top 函数

**内存**

- [ ] 热路径数据 **mlock / 大页 / 启动时 pre-fault**
- [ ] 多线程共享计数器无 **false sharing**（cache line 对齐）

**发单 & 收尾**

- [ ] 订单通道 **长连接**；`TCP_NODELAY` 等已设
- [ ] 日志、监控、持久化在 **cold path** 或异步，不挡 tick
- [ ] 实盘启动 **无 shell**；固定 daemon + 预热（连接、页、cache）

#### D. 排障顺序（别跳步）

```
1. 端到端计时 — tail 变长还是整体变慢？
2. workload — 哪一段逻辑变重（新功能？新分支？）
3. resource — CPU/softirq/丢包/NUMA/page fault 哪个亮红灯
4. 只优化测量到的瓶颈（阿姆达尔；→ CSAPP 1.9）
```

→ 方法论：[Ch 2](../chapter-02-methodologies/) · 工具：[Ch 4](../chapter-04-observability-tools/) · [Ch 13 perf](../chapter-13-perf/)

---

← [本章导读](../README.md)
