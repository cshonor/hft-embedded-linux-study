## 2.1–2.3 核心概念与术语

> **本章定位：** 建立性能分析的 **统一语言** — 后面 USE、RED、延迟分解都靠这套词。  
> HFT 里别说「慢」，要说 **平均延迟 / P999 长尾 / 抖动**；别说「忙」，要说 **利用率 / 饱和度**。

### HFT · 口语 → 标准术语（团队对齐用）

| 口语 / 模糊说法 | 标准术语 | HFT 里具体指什么 |
|-----------------|----------|------------------|
| 「策略慢了」 | **Latency / Response time** | 须拆：**Mean**（压测基线）、**P99/P999**（tail）、**Max**（单次灾难）、**Jitter**（抖动 = 相邻 tick 延迟差或 P99 波动） |
| 「行情多、单多」 | **Workload 工作负载** | 订单流量 + 撤单 + 行情 tick 的 **组合**：包率、订单形状、burst 时段 — 不是单一 QPS 数字 |
| 「CPU/网卡很忙」 | **Utilization 利用率** | 资源 **忙碌时间占比**：`mpstat` %usr、网卡带宽占用率 — **忙不忙** |
| 「排队、堵了」 | **Saturation 饱和度** | **超出即时处理能力后的排队**：run queue、NIC TX queue、锁等待、softirq 积压 — **等多久** |
| 「卡在哪」 | **Bottleneck 瓶颈** | 限制端到端的最慢环节 — 用 **延迟分解** 找最长段（→ [2.5](./section-2.5-性能分析方法论.md)） |
| 「每秒处理多少」 | **Throughput 吞吐量** | ticks/s、orders/s、packets/s |
| 「丢包、错了」 | **Errors 错误** | USE/RED 里的 **E** — `ethtool` drop、拒单、checksum 错 |

**和 Ch1 的衔接：**

```
Workload（订单流组合）  →  自上而下：程序在干什么？
Utilization / Saturation  →  自下而上：资源够吗、排队多长？
Latency 三分法（Mean/P999/Max）  →  [Ch1.6](../chapter-01-intro/notes/section-1.6-延迟指标与读法.md)
```

**为什么要统一：** 和团队 / 券商 / 运维调优时，同一词指同一指标 — 避免「慢」到底是 Mean 还是 P999、「忙」是利用率 80% 还是 TX 队列已满。

### 术语一句话 → 排查方向（不再纠结主观「慢」）

| 团队里说 | 大家立刻懂 | 先往哪查 | 通常 **不是** |
|----------|------------|----------|----------------|
| **「IOPS 低」** | 磁盘/存储 **每秒 I/O 次数**没跑满 | 容量、盘是否瓶颈、日志是否误上热路径 | **不是**「策略代码算得慢」 |
| **「P99 延迟飙了」** | **最慢 1%** 订单/tick 变差 | tail：调度、IRQ、NUMA、锁、trace 拆单笔 | **不是**「平均表现一直差」 |
| **「P999 / Max 超 SLO」** | 极端行情下有 **单笔卡死** | [Ch1.7 追踪](../chapter-01-intro/notes/section-1.7-观测工具四层递进.md) 抓那一次 | 只抠 Mean 无意义 |
| **「利用率 90%」** | CPU/网卡 **忙的时间占比** | 是否该绑核、是否多线程可扩展 | 还不等于「已排队」— 要看 **饱和度** |
| **「饱和度高了」** | TX 队列 / run queue **在等** | 加容量、减争用、削 peak workload | 不是单纯「再优化算法」能搞定 |

```
「慢」           →  先问：Mean？P99？Max？哪一段？
「IOPS 低」      →  资源吞吐没吃满，查 workload 是否根本没打满盘/网卡
「P99 飙了」     →  tail 问题，查极端行情窗口 + trace，不是重写整条策略
```

**HFT 价值：** 统一语言 = **指标 + 排查方向** 一次说清 — 会议里直接进工具链，不花在「你觉得慢我觉得不慢」上。

### 关键术语（原书 + HFT）

| 术语 | 含义 | HFT 举例 |
|------|------|----------|
| **IOPS** | 每秒 I/O 操作数 | 日志盘、持久化；**IOPS 低** = 吞吐能力未吃满，多指 **资源侧** 非逻辑慢 |
| **Throughput** | 单位时间完成的工作量 | ticks/s、orders/s |
| **Response time / Latency** | 一次操作从发起到完成的时间 | tick→signal、signal→exchange ACK；**须报 P99/Max，不单报 Mean** |
| **Utilization** | 资源忙碌时间占比 | CPU %、网卡带宽占用 — **处理交易时的饱和程度** |
| **Saturation** | 超出即时处理能力后的排队程度 | run queue、NIC TX queue、锁等待 — **利用率高的下一层** |
| **Bottleneck** | 限制整体性能的最慢环节 | 单核打满、跨 NUMA、TCP 栈 |

### 术语 → 常用命令（HFT 速查）

> 术语说完要能 **落到命令** — 下面按指标列生产/压测第一反应；热路径磁盘常可跳过。

| 术语 | 看什么 | 常用命令 | 备注 |
|------|--------|----------|------|
| **IOPS** | 磁盘每秒读写次数 | `iostat -dx 1` | **r/s、w/s**；HFT 热路径少碰盘，多用于 **日志/NVMe 冷路径** |
| | 磁盘极限 IOPS | `fio`（lab） | 压测签收容量，非实盘观测 |
| **Throughput** | 网卡带宽 / pps | `sar -n DEV 1` | **rxkB/s、txkB/s**；行情/发单网卡 |
| | 磁盘吞吐 | `dd` / `fio` | 冷路径或回放盘；`dd` 粗测 |
| | 应用层 | 自建 counter | ticks/s、orders/s — **最贴近业务** |
| **Latency / 响应时间** | 系统调用耗时 | `perf trace -s` | 跟踪 syscall 时间线 |
| | 函数级延迟 | **bpftrace** / eBPF | 热路径、单笔 tail（→ [Ch1.7](../chapter-01-intro/notes/section-1.7-观测工具四层递进.md)） |
| | 端到端 P99/Max | 应用 histogram + 硬件打点 | 比纯 `perf` 更贴 tick→trade |
| **Utilization** | CPU 各核占比 | `mpstat -P ALL 1` | **%usr / %sys / %soft / %idle** |
| | 网卡带宽占用 | `ethtool -S eth0` | 结合 `sar -n DEV` 看是否打满链路 |
| | 进程级 | `pidstat 1` | 哪个策略 PID 吃 CPU |
| **Saturation** | CPU 运行队列 | `vmstat 1` | **r 列** — 可运行线程排队长度 |
| | TCP 连接积压 | `ss -s` | 连接数、重传相关摘要 |
| | 网卡 drop / 背压 | `ethtool -S` | `rx_missed_errors`、drop |
| | 锁 / softirq | `perf stat`、bpftrace | 饱和度在 **等锁/等中断** 时 |
| **Bottleneck** | 热点函数（第一眼） | `perf top -p <pid>` | 生产 **短窗口**、低频 |
| | 固化分析 | `perf record -g` → **火焰图** | 找「宽平台」函数，定嫌疑热路径 |
| **Errors** | 网卡/内核错误 | `ethtool -S`、`dmesg` | USE 里的 **E** |

**固定套路（和你说的顺序一致）：**

```
1. mpstat / vmstat / ethtool  →  Utilization + Saturation（resource 扫盲）
2. sar -n DEV                 →  网卡 Throughput
3. perf top                   →  Bottleneck 嫌疑函数
4. perf record + 火焰图       →  固化 workload 热点
5. perf trace / bpftrace      →  Latency tail 拆 syscall/函数
```

**HFT 注意：**

- 热路径：**mpstat、ethtool、perf** 优先；`iostat`/`dd` 除非怀疑日志/回放盘
- 实盘 `perf`：**采样别太重** — 与 [Ch1 观测 vs 实验](../chapter-01-intro/notes/section-1.8-实验与微观宏观基准.md) 一致
- P99/Max：**应用内分位数** + trace 抓 spike，不单靠 `perf top` 平均值

→ 工具详解：[Ch 4 观测工具](../../chapter-04-observability-tools/) · [Ch 13 perf](../../chapter-13-perf/) · [Ch 15 BPF](../../chapter-15-bpf/)

### 时间尺度（Time Scales）— 先圈「战场」，再用术语精准定位

**两层用法：**

```
① 时间尺度  →  订单延迟落在哪个数量级？先圈定排查的「战场范围」
② 专业术语  →  在该量级上用 Throughput / RTT / Saturation 等量化、拆到具体组件
```

系统各组件时间跨度极大；建立**数量级直觉**比背参数更重要。

Gregg 经典类比：若 **1 CPU 周期 ≈ 1 秒**，则：

| 事件 | 真实量级 | 放大后感受 |
|------|----------|------------|
| L1 命中 | ~1 ns 级（数周期） | **~1 秒** — 相对「瞬间」 |
| L2 / L3 未命中 | ~10～40 ns | **数分钟** |
| 主存访问 | ~100 ns | **~6 分钟** |
| 网络 RTT | ms 级 | **~数年** |

#### HFT · L1 命中是什么

**大白话：** CPU 要读的数据 **刚好在一级缓存（L1）里**，不用去更慢的 L2、L3 或主存找。

| 概念 | 说明 |
|------|------|
| **L1 缓存** | 离 CPU 核心 **最近、最快** 的存储层，访问延迟 **纳秒级** |
| **类比位置** | 上表「1 CPU 周期 ≈ 1 秒」里，L1 命中 ≈ **1 秒档** — 快，但和主存、网络比仍是「近场」 |
| **L1 未命中** | 逐级去 L2 → L3 → 主存，延迟 **翻几倍到几十倍** — 单条撮合从百 ns 飙到 μs 档 |

**HFT 里为什么在乎：**

```
撮合热路径反复碰的数据：订单簿档位、最优买卖价、账户持仓、风控计数器
  → 布局紧凑、访问局部、绑核少迁移
  → 尽量让它们在 L1 里「常驻」
  → 把单条订单处理压在纳秒～百纳秒档

一旦 L1 不命中（结构体太大、指针乱跳、false sharing、跨 NUMA）
  → 同样逻辑多等几十～上百 ns
  → 在抢 tick、抢排队位置时，直接吃掉时间优势
```

**和时间尺度的关系：** 只有当你已锚到 **路径 B**（撮合单步 ~500ns、战场在 CPU/cache）时，才值得抠 L1；若端到端瓶颈在 **2ms 网络**（路径 A），先别纠结 L1 命中率。

→ cache 层级与 `perf stat`：[Ch 6](../../chapter-06-cpus/) · [Ch 4 PMC](../../chapter-04-observability-tools/notes/section-4.3-核心观测数据源.md)

#### HFT · 延迟落在哪一档，就查哪一档的术语

| 你量的端到端延迟 | 战场在哪 | 优先用的术语 / 查什么 | **别死磕** |
|------------------|----------|----------------------|------------|
| **μs 级**（tick→信号） | CPU / cache / 用户态热路径 | Utilization、Bottleneck、`perf`、绑核、false sharing | 交换机 ms 抖动（还轮不到） |
| **百 μs～ms 级**（成交/ACK 变慢） | 内核栈、网卡、队列、对端 | **Throughput**、**RTT**、TCP **Saturation**、`ss -s`、`ethtool -S` | 纳秒级 cache 微调 |
| **ms+ 级** | 网络路径、交易所、拥塞 | RTT、重传、队列饱和、路径变更 | 单机 CPU 指令级优化 |

**反例 — 捡芝麻丢西瓜：**

```
只优化纳秒级 cache 命中率
  却没解决毫秒级网络抖动 / 队列饱和
  → 对 HFT 端到端几乎无感 — 时间尺度锚错了
```

#### 实战走一遍：先锚量级，再拆组件

> **口诀：** 先量一笔落在哪档 → 再选术语 + 命令，**别跳档优化**。

**路径 A · 发单→ACK 慢了（毫秒级网络战场）**

```
1. 锚量级
   tcpdump / 应用时间戳 / 交易所 ACK 日志
   → 例：应用发出 → 收到 ACK = 2ms
   → 锚定：**毫秒级网络尺度** — 不查纳秒 CPU cache

2. 定术语 + 命令（Saturation / Throughput / Errors）
   ss -s              →  TCP 队列是否积压、连接状态摘要
   ethtool -S eth0    →  网卡 drop、rx_missed_errors（Errors）
   sar -n DEV 1       →  带宽是否打满（Throughput）
   ss -ti / ping      →  RTT、重传（Latency 网络段）

3. 结论方向
   队列满 / 丢包 / RTT 抖  →  迁 IRQ、换路径、控 peak — 不是改撮合 if-else
```

**路径 B · 撮合单步逻辑 500ns（纳秒～微秒 CPU 战场）**

```
1. 锚量级
   应用内 rdtsc / 分段 histogram
   → 例：单步撮合 = 500ns
   → 锚定：**CPU / cache 热路径** — 此时才值得抠指令与 cache

2. 定术语 + 命令（Utilization / Bottleneck）
   perf top -p <pid>           →  热点函数（Bottleneck）
   perf record -g → 火焰图     →  固化宽栈
   perf stat -e cache-misses   →  L3 / cache 行为（ns～百 ns 档）

3. 结论方向
   某函数宽、cache miss 高  →  改数据结构、对齐、绑核 — 不是先怪交换机
```

| 你先量到什么 | 锚在哪 | 下一步工具 | 别误闯 |
|--------------|--------|------------|--------|
| 发单→ACK **2ms** | ms · 网络 | `ss -s`、`ethtool -S`、`sar -n DEV` | `perf` 抠 cache |
| 撮合一步 **500ns** | ns～μs · CPU | `perf top`、火焰图、`cache-misses` | `tcpdump` 查 RTT |

**和前面术语表的关系：**

| 步骤 | 做什么 |
|------|--------|
| 1 | 看 P99/Max 落在 **ns / μs / ms** 哪档 → **定战场** |
| 2 | 在该档选 **Utilization / Saturation / Throughput / Latency** → **定术语** |
| 3 | 用 [术语→命令](#术语--常用命令hft-速查) 表落地 → **定工具** |

→ 硬件数量级：[04-Hennessy](../../../04-Computer-Architecture-6th/) · NUMA/cache：[Ch 6/7](../../chapter-06-cpus/) · 网络 ms 档：[Ch 10](../../chapter-10-network/)

### 性能权衡（Trade-Offs）

- **好 / 快 / 便宜 — 三选二**：共置机房、专线、低延迟网卡 = 用成本换延迟。
- **CPU ↔ 内存**：用内存缓存换 CPU（行情快照、order book）；用 CPU 压缩/编码换内存与带宽。
- **延迟 ↔ 吞吐量**：批处理提高吞吐但增加单条延迟；HFT 通常优先尾延迟。

### 负载 vs 架构（Load vs. Architecture）

| 类型 | 含义 | 典型表现 |
|------|------|----------|
| **负载过高** | 请求量超过当前容量，排队变长 | 行情暴增、撮合高峰 |
| **架构问题** | 设计无法扩展或争用严重 | 单线程无法吃满多核、全局锁、False sharing |

**区分方法：** 同样负载下，换更好硬件若仍慢 → 更像架构；负载降下去就恢复 → 更像容量。

### 已知的未知（Known-Unknowns）

性能分析 = 不断把 **「未知的未知」** 变成 **「已知的未知」**（知道该查什么，但还没查），再变成可验证的结论。

```
未知 → 列假设 → 选指标/工具 → 证实/排除 → 下一层
```

与 Ch 1 **观测 vs 实验** 一致：先观测定位，再可控实验验证。

### 术语 → 后续方法论（预告）

| 术语组 | 后面用在哪 |
|--------|------------|
| Utilization / Saturation / Errors | **USE** — 每个资源问 U/S/E（→ [附录 A](../../appendix-A-USE方法Linux.md)） |
| Rate / Errors / Duration | **RED** — 每个服务问请求率、错误、耗时 |
| Latency 分解 | **延迟分解** — tick→decode→signal→send 各段 Duration |

→ [2.4 两种视角](./section-2.4-两种分析视角.md) · [2.5 方法论](./section-2.5-性能分析方法论.md)

---

← [本章导读](../README.md)
