# Ch 12 基准测试 · Benchmarking

> **Systems Performance 2nd** · Brendan Gregg · **选读**

> 本章定位：**基准测试「出人意料地棘手」** — 跑分高 ≠ 生产快。Gregg 不罗列工具了事，而是教 **如何设计实验、控制变量、结合观测、拷问报告**。Ch 8/9/10 各章的 fio/iperf 微基准，必须在本章方法论框架下解读。  
> **HFT：** 微观基准（fio、iperf）只做 **capacity baseline**；策略与端到端延迟靠 **生产级 replay + 应用 span**（→ [11-HFT ch10](../11-HFT-Low-Latency-Practice/chapter-10-延迟测量与基准压测.md)）。

---

## 大白话 · 本章就四件事

> **基准测试是实验，不是电竞跑分。**

**① 为什么要测 + 为什么常失败 — 先懂局限再动手。**

- 有效基准 = 可重复、可解释、**与生产 workload 相关**。
- 失败模式：测错层、测到 cache、样本太少、环境不一致、结论过度推广。

**② 四种类型：Micro / Simulation / Replay / Industry Standard。**

- **Micro** — 单组件（CPU/盘/网）；**Replay** — 录生产再播（架构变了可能误导）。
- **Macro 行业标准** — SPEC 等；HFT 更常 **自定义 + 真实报文 replay**。

**③ 方法论：主动/被动、USE+Profile、阶梯负载、Sanity Check、统计、Checklist。**

- 压测时 **同时** CPU 火焰图 + USE — 确认瓶颈在声称的那一层。
- **Ramping load** 找拐点；**Sanity Check** 交叉验证工具是否在测你以为的东西。

**④ 拷问报告（Benchmark Questions）— 看 vendor 亮分保持批判。**

- 工作负载是什么？预热了吗？P99 呢？与你们生产像吗？

下面按原书 12.1–12.4 展开。

---

## 12.1 基准测试的背景与挑战

### 为什么要做基准测试（Reasons）

| 目的 | 例子 |
|------|------|
| **容量规划** | 新机器能扛多少行情 pps |
| **回归检测** | 策略改一行，P99 是否变差 |
| **对比选型** | NIC/内核/DPDK vs 内核栈 |
| **验证调优** | sysctl/绑核前后 |
| **SLA 证明** | 对交易所/托管方交付证据 |

### 什么是有效基准（Effective Benchmarking）

| 有效 | 无效 |
|------|------|
| 目标明确（测什么、为谁） | 「跑个 fio 看快不快」 |
| 环境文档化、可复现 | 未说明内核/驱动/邻居 |
| 与 **生产 workload 相关** | 纯 synthetic 且形状完全不同 |
| 报告 **分布**（P50/P99） | 只报一个平均值 |
| 结合 **观测** 解释瓶颈 | 只有数字无 profile |

### 为什么常失败（Benchmarking Failures）

| 失败模式 | 说明 |
|----------|------|
| **测到 cache 而非磁盘** | 文件 < RAM（Ch 8 WSS） |
| **测到空闲 CPU 而非真瓶颈** | 单线程 micro-bench |
| **冷启动 vs 稳态混淆** | JIT、cache 预热、连接建立 |
| **邻居/云 steal** | 多租户未隔离（Ch 11） |
| **工具 bug / 错误用法** | fio engine、block size 不对 |
| **过度外推** | 实验室 100G 推到生产混合流量 |

**Gregg 核心：** 基准测试 **很容易错**，或 **对生产毫无意义** — 必须理解局限性。

**HFT：** 「testpmd 跑满 100G」**不能** 直接等于「order book 更新 P99」— 中间差整应用栈。

→ Ch 1 [微观 vs 宏观](./chapter-01-简介.md)

---

## 12.2 基准测试的类型

### 微观基准测试（Micro-Benchmarking）

针对 **单一组件**、简化人工负载：

| 组件 | 常见工具 | 测什么 |
|------|----------|--------|
| CPU | `perf bench`、自定义 loop | 算力、分支 |
| 磁盘/FS | **fio** | IOPS、延迟分位 |
| 网络 | **iperf3**、netperf | 吞吐、RTT |
| 内存 | `lmbench`、`stream` | 带宽、latency |
| DPDK | testpmd | PPS |

**优点：** 隔离变量、可重复。  
**缺点：** **脱离** 真实 syscall 路径、锁、业务逻辑。

### 模拟测试（Simulation）

**合成 workload** 模仿生产特征（比例、大小、并发）— 比 raw micro 更接近真实，但仍需 **校准**（比例是否对）。

**HFT 例：** 合成 UDP 组播包率 + 固定 order book 深度 — 仍不如真实 exchange feed 字段分布。

### 重放测试（Replay）

捕获 **生产 trace** 再回放：

| 类型 | 工具/方式 |
|------|-----------|
| 块 I/O trace | `blktrace` replay |
| 网络 pcap | tcpreplay |
| 应用请求 | 自定义日志 replay |

**Gregg 警告（Ch 9 呼应）：** 若 **目标系统架构或性能特征已变**（新盘、新 FS、新网卡），replay **可能误导** — 队列行为、合并、cache 都不同。

**HFT：** 历史 tick **replay 测策略** 有价值；replay **磁盘 trace** 换 NVMe 后端时要重新录。

### 行业标准（Macro / Industry Standards）

| 套件 | 领域 |
|------|------|
| SPEC CPU / SPECjbb | 通用 / Java |
| TPC-* | 数据库 |
| 厂商 NIC 官方 benchmark | 网络 |

**特点：** 宏观、可对比、**不一定** 像你的业务 — 读方法论 + 拷问问题（12.4）。

---

## 12.3 基准测试方法论

### 被动 vs 主动基准测试

| 类型 | 做法 |
|------|------|
| **Passive** | 观察 **生产** 或 idle 系统 — 不施加人工负载 |
| **Active** | **施加** 负载（fio、压测客户端） |

**HFT：** 生产 **passive**（P99、BPF）为主；**active** 在 **staging / 裸机验收** — 勿在生产 peak 乱 fio。

### 结合观测（必做）

压测时 **同时**：

| 方法 | 确认什么 |
|------|----------|
| **USE** | 资源是否如预期饱和（Ch 2） |
| **CPU Profiling** | 热点是否在声称路径 |
| **Workload characterization** | IOPS/块大小/并发是否匹配设计 |

```
例：fio 报 500k IOPS，但 CPU 火焰图 80% 在 fio 自身
  → 测的是 fio 能力，不是「你的日志进程写盘能力」
```

→ Ch 4–6 工具 · Ch 13 perf

### 自定义基准（Custom Benchmarks）

当 fio/iperf **无法代表** 业务时 — **自己写**：

- 解码真实 FIX/SBE 报文 → 更新 book → 发单 stub
- 固定 seed、固定输入文件 — **可复现**

**HFT 最佳实践：** Micro 工具 baseline + **自定义 replay harness** 报 **端到端 span**。

→ [11-HFT ch10](../11-HFT-Low-Latency-Practice/chapter-10-延迟测量与基准压测.md)

### 阶梯式施加负载（Ramping Load）

```
并发 1 → 2 → 4 → 8 → … → 直到
  - 延迟急剧恶化（拐点）
  - 错误率上升
  - 吞吐平台
```

**找：** 可扩展性上限、排队开始的位置 — 与 Ch 2 排队论、Ch 10 backlog 呼应。

**HFT：** 行情 pps 阶梯 + 记录 **P99 tick latency** — 找 **单核/单 NUMA 上限**。

### 合理性检查（Sanity Check）

| 检查 | 做法 |
|------|------|
| 数量级 | 10G 网卡不应测出 1Tbps |
| 交叉工具 | fio vs dd vs 应用写 — 趋势一致 |
| 系统计数器 | `iostat`/`ip -s` 与 fio 报告 |
| 瓶颈位置 | profile 是否在预期层 |
| 可重复 | 连跑 3 次，方差可接受 |

### 统计分析（Statistical Analysis）

→ Ch 2 [统计与可视化](./chapter-02-方法论.md#210-统计与可视化)

| 要点 | 做法 |
|------|------|
| 别只报 mean | **P50/P99/P999**、直方图 |
| 离群值 | 单独分析，不默默删掉 |
| 样本量 | 足够长（覆盖 GC、cache 稳态） |
| 置信 | 多次运行、固定环境 |

### 基准测试检查清单（Benchmarking Checklist）

**测试前：**

- [ ] **目标** 一句话：测什么、成功标准是什么
- [ ] **Workload** 描述：读/写比、大小、并发、时长
- [ ] **环境** 文档：硬件、内核、驱动、邻居、挂载选项
- [ ] **生产相关性** 说明：为何此负载代表生产
- [ ] 观测工具就绪：perf、sar、sadc、BPF
- [ ] 基线：未调优 vs 调优后 **分开测**

**测试中：**

- [ ] **预热**（JIT、cache、连接）— 区分冷/热
- [ ] 同时 **USE + profile**
- [ ] 记录 **系统计数器**（非仅工具 stdout）
- [ ] **Ramping** 或至少多档并发

**测试后：**

- [ ] **Sanity check** 通过
- [ ] 报告 **分布** + 均值
- [ ] 结论 **限定范围** — 不外推
- [ ] 存档：命令行、配置、fio job 文件、内核版本

---

## 12.4 基准测试拷问（Benchmark Questions）

看到别人（尤其 **厂商**）的亮眼报告时，问：

### 工作负载与环境

| 问题 | 为何重要 |
|------|----------|
| 测试的 **具体 workload** 是什么？ | 「500k IOPS」何种块大小、随机/顺序、队列深度？ |
| 与 **我们生产** 有多像？ | 不像则数字仅供参考 |
| **硬件/软件栈** 完整配置？ | 特定 NIC 驱动、特定内核 |
| 是否 **专用机** / 无邻居？ | 云 shared vs 裸金属 |
| **预热** 了吗？测的是冷还是热？ | 第一次 vs 稳态差数量级 |

### 指标与统计

| 问题 | 为何重要 |
|------|----------|
| 报的是 **平均** 还是 **P99/P999**？ | HFT 看 tail |
| 测试 **时长**？样本量？ | 太短无统计意义 |
| 有无 **错误/丢包/重传**？ | 吞吐高但丢包无意义 |
| **成本/瓦数**？ | 同吞吐不同功耗 |

### 方法与可复现

| 问题 | 为何重要 |
|------|----------|
| 工具与 **完整命令行**？ | 能否复现 |
| 是否 **drop_caches** / O_DIRECT？ | 测 cache 还是盘 |
| 瓶颈 **profile** 在哪？ | 是否测到 intended layer |
| 与 **竞品** 测试条件是否一致？ | 苹果 vs 橙子 |

### HFT _vendor 拷问加项

- 测的是 **kernel 栈还是 DPDK**？RX 队列几条？绑核了吗？
- **报文大小** 是否像真实行情（小 UDP 多包）？
- **RTT** 测量点在哪（NIC timestamp vs 应用）？
- 是否包含 **交易所共置 RTT** 还是实验室回环？

**Gregg 精神：** 极亮眼的数字 **默认可疑** — 用问题拆穿或验证。

---

## HFT 工具与场景对照

| 场景 | 推荐类型 | 工具/方式 |
|------|----------|-----------|
| 日志盘验收 | Micro | fio `direct=1`（Ch 8/9） |
| 网络带宽 baseline | Micro | iperf3（Ch 10） |
| 网卡 PPS 上限 | Micro | testpmd / pktgen |
| 策略回归 | Simulation / Replay | 历史 tick replay + P99 |
| 端到端 SLA | Custom + Passive | span timestamp（ch10） |
| 整机采购 | Macro + Micro | 行业标准 + 自有 replay |

---

## 本章 Checklist

- [ ] 能说清 **四种基准类型** 及各自陷阱
- [ ] 主动压测时 **同时 USE + profile**
- [ ] 磁盘/FS 测试考虑 **WSS 与 drop_caches**
- [ ] 报告 **P99** 而非仅 mean
- [ ] 会用 **12.4 拷问清单** 读 vendor 报告
- [ ] HFT 区分 **micro baseline** vs **生产 replay**

---

## HFT 精读捷径（Ch 12 在路线中的位置）

```
Ch 2  统计、延迟分解
Ch 8–10  fio / iperf 工具（各章微基准）
Ch 12  基准方法论（本章：如何测、如何信）
  → Ch 13 perf（压测时 profile）
  → 11-HFT ch10（量化端到端压测落地）
```

**本章最小行动集：**

1. 为 **日志 NVMe** 写一份 **fio job 文件** + 环境说明 — 存档可复现。
2. 对策略 replay 报 **P50/P99/P999** — 不只 average tick time。
3. 读任意一份 NIC 厂商 benchmark — 用 **12.4 问题** 标红 3 个缺失项。

**Gregg 本章金句（HFT 版）：**

> 基准测试 **出人意料地棘手** — 「跑分很高、生产很差」是常态，不是意外。  
> **Micro 测容量，Replay 测行为，Profile 证明瓶颈** — 三者缺一不可。

---

## 相关章节

- 上一章：[chapter-11-云计算.md](./chapter-11-云计算.md)
- 下一章：[chapter-13-perf性能分析.md](./chapter-13-perf性能分析.md)
- 方法论：[chapter-02-方法论.md](./chapter-02-方法论.md)
- fio / FS：[chapter-08-文件系统.md](./chapter-08-文件系统.md)
- fio / 磁盘：[chapter-09-磁盘.md](./chapter-09-磁盘.md)
- iperf / 网络：[chapter-10-网络.md](./chapter-10-网络.md)
- HFT 压测：[11-HFT ch10](../11-HFT-Low-Latency-Practice/chapter-10-延迟测量与基准压测.md)
- 全书目录：[OUTLINE.md](./OUTLINE.md)
