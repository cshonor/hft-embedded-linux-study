## 2.4 两种分析视角

> **出处：** Gregg《性能之巅》· 性能分析有两套 **互补** 路径 — **不必二选一**，故障排查、容量规划通常 **结合使用**。  
> **HFT 实操要点：** 两者 **不是割裂的两套世界**，而是 **有先后顺序的接力** — 先 Workload 抓业务真相，再 Resource 挖底层原因。

```
自上而下  Workload Analysis   业务 → 系统   「哪个环节、影响多大？」
自下而上  Resource Analysis   资源 → 业务   「为什么会这样？」
         ────────────  先上后下，不是二选一  ────────────
```

→ 术语对照：[2.1](./section-2.1-HFT术语与团队对齐.md) · [2.2 命令](./section-2.2-术语与命令速查.md) · [2.3 走查](./section-2.3-时间尺度与排查走查.md) · [2.5 分层埋点](./section-2.5-性能分析方法论.md) · [附录 A](../../appendix-A-USE方法Linux.md)

---

### 先 Workload，后 Resource — 一句话

| 阶段 | 问什么 | 靠什么 | 优先级 |
|------|--------|--------|--------|
| **① 业务定位** | 问题出在 **哪个环节**、**影响多大** | 代码埋点 counter / histogram、Prometheus / Grafana、RED 指标 | **最先** |
| **② 底层下钻** | **为什么** 会出问题 | `perf`、`ss`、`ethtool`、`tcpdump`、`dmesg`、USE 清单 | **定位到进程/链路之后** |

**不是不用 Linux 命令** — 是 **靠后**。Workload 的核心是先确立 **业务维度的真相**：「每秒 tick 掉了多少」「订单 P99 从 10 μs 涨到 100 μs」「报单 reject 率突增」—— 这些必须靠 **开发阶段埋好的点** 和 **应用层监控** 先确认；等 narrowed 到「某个进程、某段链路、某个接口」，再让底层工具上场解释机制。

```
Grafana / 应用 histogram     →  「报单 reject 率突增」     （Workload · 第一步）
        ↓ 锁定接口 / 进程 / 时段
tcpdump 该接口流量 · dmesg     →  内核/网络是否异常         （Resource · 第二步）
perf -p <pid> · ss -s          →  CPU 耗在哪、有无重传      （Resource · 第三步）
```

---

### 一、资源分析（Resource Analysis · 自下而上）

| | |
|--|--|
| **适用** | 系统管理员、SRE、平台工程、运维 — **也在 Workload 锁定嫌疑之后** |
| **看什么** | CPU、内存、磁盘 I/O、网卡、总线 — **USE**：Utilization / Saturation / Errors |
| **核心指标** | 利用率、队列饱和度、硬件 drop/error、软中断、重传、上下文切换 |
| **工具优先级** | **在「哪个进程 / 哪条链路有问题」已知之后** 再开 `mpstat` / `perf` / `ethtool` |

**典型问题（回答「为什么」）：**

1. 策略进程是不是 **某个核 100%**、**softirq 跑满**？
2. 网卡有没有 **drop**、硬件 error？
3. 内存 **带宽 / NUMA** 是否成为瓶颈？
4. TCP 有没有 **重传**（`ss`）？内核有没有 **OOM / 驱动报错**（`dmesg`）？

**配套工具：**

| 资源 | 命令 | 何时用 |
|------|------|--------|
| CPU | `mpstat -P ALL 1`、`perf top -p <pid>` | Workload 指向某 PID 后 |
| 内存 / 队列 | `vmstat 1` | run queue 飙、P99 tail 时 |
| 磁盘 | `iostat -dx 1` | 怀疑日志/回放盘（HFT 热路径较少） |
| 网络 | `ss -s`、`ethtool -S`、`tcpdump` | reject/延迟异常 **且** 怀疑网络段时 |
| 内核 | `dmesg` | drop、驱动、OOM 线索 |

→ HFT 速查：[2.2 术语→命令](./section-2.2-术语与命令速查.md)

---

### 二、工作负载分析（Workload Analysis · 自上而下）

| | |
|--|--|
| **适用** | 应用开发、策略/交易工程、产品性能分析 |
| **看什么** | 业务请求、服务行为 — **请求量、延迟分布、成功/错误比** |
| **核心指标** | tick 速率、订单 **P99/P999** 延迟、**reject / timeout 错误率**、各段事务耗时 |
| **工具优先级** | **① 代码埋点**（counter、histogram）→ **② Prometheus / Grafana** → ③ 追踪（bpftrace / trace） |

**典型问题（HFT）— 全部从业务指标入口：**

1. 行情每秒多少 tick？（counter / RED **Rate**）
2. 下单 / 报单接口 **P99** 多少 μs？（histogram / RED **Duration**）
3. 报错是 **timeout** 还是 **reject**？（RED **Errors**）
4. 哪个接口、哪条链路在 **什么时段** 异常？

**配套工具（按优先级）：**

| 优先级 | 层级 | 工具 |
|--------|------|------|
| **P0** | 应用埋点（开发期写入） | atomic **counter**（ticks/s、orders/s）、**histogram**（P50/P99/Max）、段间计时 |
| **P1** | 业务监控 | **Prometheus + Grafana**、自建 dashboard、告警（reject 率、P99 阈值） |
| **P2** | 追踪 / 压测 | bpftrace、eBPF、Jaeger（若已接）、[Ch 12 基准](../../chapter-12-benchmarking/) |
| **P3** | 短窗口系统采样 | 生产低频 `perf` — **在 Workload 已锁定嫌疑进程之后** |

→ tick 速率两路验证（埋点为主、`tcpdump` 交叉）：[2.5 分层埋点](./section-2.5-性能分析方法论.md#第一步每秒多少-tick两路交叉验证)  
→ 延迟怎么读：[Ch1.6](../chapter-01-intro/notes/section-1.6-延迟指标与读法.md) · 观测四层：[Ch1.7](../chapter-01-intro/notes/section-1.7-观测工具四层递进.md)

---

### 三、对比与协同 — 互补，不是对立

| 维度 | 工作负载分析（先） | 资源分析（后） |
|------|---------------------|----------------|
| **起点** | 业务核心指标：tick 速率、P99、reject 率 | 已锁定的进程 / 网口 / 时段 |
| **回答** | **哪个环节**、**影响多大** | **为什么**（CPU 软中断？丢包？内存带宽？） |
| **主力工具** | 代码 counter、histogram、Prometheus / Grafana | `perf`、`ss`、`ethtool`、`tcpdump`、`dmesg` |
| **开发阶段** | **必须** 预埋埋点 — 否则线上无「业务真相」 | 一般不需改代码；用现有观测面 |
| **擅长** | 逻辑缺陷、接口慢、流量突增、错误类型 | 硬件瓶颈、内核故障、容量饱和 |

**Ch1 双视角 · 接力版：**

```
① Workload  tick→parse→signal→send  哪段 P99/reject 异常？  （Grafana / histogram）
② Resource  对应阶段 CPU/NIC/队列 USE 是否异常？          （perf / ethtool / ss）
③ 对齐      慢段 ↔ 饱和资源 → 改代码 vs 加容量 vs 砍负载
```

**负载异常时的典型接力（P99 飘高）：**

```
应用监控：订单 P99  10 μs → 100 μs          ← Workload 确认「变坏了、变多少」
段间 histogram：慢在 signal→send             ← Workload 锁定环节
perf -p <gateway_pid>                       ← Resource 看 CPU 耗在哪
ss -s / ethtool -S                          ← Resource 看重传、drop
```

---

### 四、实战：先走哪条路？

| 入口 | 第一步（Workload） | 第二步起（Resource） |
|------|-------------------|----------------------|
| **P99 飙 / reject 突增 / tick 掉速** | Grafana / histogram 确认 **指标、幅度、时段、环节** | `perf` 嫌疑进程 · `ss` 重传 · `ethtool` drop · `dmesg` |
| **「CPU/磁盘告警打满」**（无业务监控时） | 若有可能，先补 **Rate/Errors** 看业务是否已受损 | USE 扫资源 → 关联到具体 PID / 服务 |
| **容量规划 / 硬件验收** | 定义目标 workload（ticks/s、P99 SLO） | USE 看 headroom 是否够 |

#### 完整示例：报单 reject 率突增

| 步 | 视角 | 动作 | 得到什么 |
|----|------|------|----------|
| 1 | **Workload** | 应用监控 / Grafana：`order_submit` **reject_rate** 曲线 | 「09:31 reject 从 0.1% → 8%」— **问题存在、可量化** |
| 2 | **Workload** | 同面板看 **Rate**（报单/s）、**Duration P99**、错误码分类 | 流量是否 burst？是 **风控 reject** 还是 **交易所 reject**？ |
| 3 | **Workload** | 段间计时：signal→serialize→send 哪段变长 | 锁定 **网关进程 / 报单链路** |
| 4 | **Resource** | `tcpdump -i eth1 'host <exchange>' -c 500` 抓报单口 | 报文是否发出、RTT 是否变长 |
| 5 | **Resource** | `ss -s`、连接级重传；`dmesg \| tail` | 内核/TCP 是否异常 |
| 6 | **Resource** | `perf record -p <gateway_pid> -g -- sleep 30` | CPU 热点：序列化？锁？syscall？ |

**反例：**

| ❌ | 为什么错 |
|----|----------|
| 一报警就 `mpstat`，不看 reject 曲线 | 不知道 **业务受损程度** 和 **哪条接口** |
| 只有 `top`，从不埋 P99 / counter | 没有 **业务维度真相**，只能猜 |
| 只看 Grafana，从不 `perf` / `ethtool` | 知道「慢了」但不知道 **CPU 还是网卡** |
| 开发期零埋点，指望线上 `tcpdump` 补全 | `tcpdump` **解释不了** P99 从 10→100 μs 发生在哪段 |

**HFT 合并用法（与 [2.3 排查顺序](./section-2.3-时间尺度与排查走查.md#排查顺序-vs-优化优先级) 一致）：**

```
1. Workload：Grafana / histogram — tick→parse→signal→send 各段 Rate、Duration、Errors
2. 锁定环节 / 进程 / 时段
3. Resource：USE + perf / ethtool / ss / tcpdump / dmesg — 解释「为什么」
4. 对齐：慢段 ↔ 饱和/错误资源 → 改代码 / 加容量 / 砍负载
```

→ 分层埋点套路：[2.5](./section-2.5-性能分析方法论.md#hft-实战业务链路分层埋点--工具串连) · 工具地图：[Ch 4](../../chapter-04-observability-tools/) · `perf`：[Ch 13](../../chapter-13-perf/)

---

← [本章导读](../README.md)
