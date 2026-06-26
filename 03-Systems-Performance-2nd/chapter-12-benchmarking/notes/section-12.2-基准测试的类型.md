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


---

← [本章导读](../README.md)
