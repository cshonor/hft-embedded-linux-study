## 2.4 两种分析视角

> **出处：** Gregg《性能之巅》· 性能分析有两套 **互补** 路径 — **不必二选一**，故障排查、容量规划通常 **结合使用**。

```
自上而下  Workload Analysis   业务 → 系统   「程序在干什么、慢在哪段？」
自下而上  Resource Analysis   资源 → 业务   「CPU/网卡/盘够不够、排队多长？」
```

→ 术语对照：[2.1](./section-2.1-HFT术语与团队对齐.md) · [2.2 命令](./section-2.2-术语与命令速查.md) · [2.3 走查](./section-2.3-时间尺度与排查走查.md) · [2.5](./section-2.5-性能分析方法论.md) · [附录 A](../../appendix-A-USE方法Linux.md)

---

### 一、资源分析（Resource Analysis · 自下而上）

| | |
|--|--|
| **适用** | 系统管理员、SRE、平台工程、运维 |
| **看什么** | CPU、内存、磁盘 I/O、网卡、总线 — **USE**：Utilization / Saturation / Errors |
| **核心指标** | 利用率、队列饱和度、硬件 drop/error、IOPS、吞吐、中断、上下文切换 |

**典型问题：**

1. 哪颗 CPU 核心满载 100%？
2. 网卡有没有 drop、硬件 error？
3. 磁盘 I/O 是否打满、等待队列是否过长？
4. 内存是否 Swap 抖动、OOM？

**配套工具：**

| 资源 | 命令 |
|------|------|
| CPU | `mpstat -P ALL 1`、`pidstat` |
| 内存 / 队列 | `vmstat 1` |
| 磁盘 | `iostat -dx 1` |
| 网络 | `sar -n DEV 1`、`ss -s`、`ethtool -S` |

→ HFT 速查：[2.2 术语→命令](./section-2.2-术语与命令速查.md)

---

### 二、工作负载分析（Workload Analysis · 自上而下）

| | |
|--|--|
| **适用** | 应用开发、策略/交易工程、产品性能分析 |
| **看什么** | 业务请求、服务行为 — **请求量、延迟分布、成功/错误比** |
| **核心指标** | QPS / tick 速率、P50/P95/P99/P999、超时率、reject 率、各段事务耗时 |

**典型问题（HFT）：**

1. 行情每秒多少 tick？ → **两路交叉验证**（`tcpdump -c 1000` + 解析 atomic counter）见 [2.5 分层埋点](./section-2.5-性能分析方法论.md#hft-实战业务链路分层埋点--工具串连)
2. 下单 / 撮合接口 P99 多少 μs？
3. 报错是 **timeout** 还是 **reject**？
4. 哪个接口错误率突增？

**配套工具：**

| 层级 | 工具 |
|------|------|
| 应用埋点 | histogram（Mean/P99/Max）、counter（ticks/s、orders/s） |
| 追踪 | bpftrace、eBPF、Jaeger/Zipkin（若已接） |
| 压测 | 可控 workload 复现（→ [Ch 12 基准](../../chapter-12-benchmarking/)） |
| APM | 生产短窗口 `perf`、RED 指标 |

→ 延迟怎么读：[Ch1.6](../chapter-01-intro/notes/section-1.6-延迟指标与读法.md) · 观测四层：[Ch1.7](../chapter-01-intro/notes/section-1.7-观测工具四层递进.md)

---

### 三、对比与协同

| 维度 | 资源分析（自下而上） | 工作负载分析（自上而下） |
|------|----------------------|--------------------------|
| **起点** | 底层硬件 / OS 资源 | 上层业务请求、用户行为 |
| **思考顺序** | 资源饱和 → 拖累业务 | 业务慢 → 向下找瓶颈资源 |
| **擅长定位** | 硬件瓶颈、内核故障、容量水位 | 逻辑缺陷、接口慢、流量突增、业务报错 |
| **典型场景** | 容量规划、CPU/盘打满、硬件故障 | 用户反馈卡、交易延迟飙、接口报错 |

**Ch1 双视角回顾：**

```
Workload 视角  →  tick→parse→signal→send 各段在干什么？
Resource 视角  →  对应阶段 CPU/NIC/队列是否 USE 异常？
```

---

### 四、实战：先走哪条路？

| 入口 | 优先视角 | 然后 |
|------|----------|------|
| **「业务慢 / P99 飙了」** | **自上而下** Workload | 查接口延迟、错误率 → 调用链 / 延迟分解 → 再查底层资源 |
| **「CPU/磁盘告警打满」** | **自下而上** Resource | USE 定位饱和资源 → 关联进程 / 业务流量 |
| **复杂线上故障** | **双视角联动** | 业务侧延迟↑ + 同时查资源饱和/drop — 区分代码问题 vs 资源瓶颈 |

**HFT 合并用法（与 [2.3 排查顺序](./section-2.3-时间尺度与排查走查.md#排查顺序-vs-优化优先级) 一致）：**

```
1. Workload：tick → parse → signal → send 各段 latency（应用 histogram + trace）
2. Resource：对应阶段 CPU、cache、NIC 队列（USE + mpstat/ethtool/perf）
3. 对齐：慢的阶段 ↔ 饱和/错误的资源 — 再定是改代码还是加容量
```

**反例：** 只盯 `mpstat` 不看 P99 — 不知道慢在撮合还是网络；只盯 P99 不看 `ethtool` drop — 可能漏硬件饱和。

→ 工具地图：[Ch 4](../../chapter-04-observability-tools/) · `perf`：[Ch 13](../../chapter-13-perf/) · BPF：[Ch 15](../../chapter-15-bpf/)

---

← [本章导读](../README.md)
