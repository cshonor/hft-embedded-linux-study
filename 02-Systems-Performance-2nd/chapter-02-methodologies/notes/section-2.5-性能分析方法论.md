## 2.5 性能分析方法论

### 反面方法论（Anti-Methods）— 避免

| 名称 | 表现 | HFT 版「踩坑」 |
|------|------|----------------|
| **路灯反面法** | 只用自己熟悉的工具，缺乏目标 | 只会 `top`，从不看 tick-to-trade 分段 |
| **随机修改** | 盲改参数看问题是否消失 | 乱调 `-O3`、绑核、TCP 参数，无 baseline |
| **责怪他人** | 无数据甩锅网络/交易所/DB | 「肯定是交易所慢」— 没有 RTT 分解证据 |

**共同问题：** 无假设、无度量、无复现 → 浪费共置窗口时间。

---

### USE 方法（The USE Method）— 资源分析核心

对**每个**硬件或软件资源，依次问三件事：

| 字母 | 英文 | 问什么 |
|------|------|--------|
| **U** | Utilization | 资源有多忙？ |
| **S** | Saturation | 排队/等待有多严重？ |
| **E** | Errors | 有没有错误？ |

**清单思维：** CPU、每个 CPU、内存、容量、网络接口、协议栈、磁盘… 逐项过，**不漏资源**。

Linux 实操清单 → [附录 A](../../appendix-A-USE方法Linux.md)  
HFT 重点资源：CPU（含 NUMA）、内存带宽、网卡/NAPI 队列、锁与 run queue — 磁盘/文件系统多为 ⚪。

---

### RED 方法（The RED Method）— 服务 / 微服务

对每个**服务**（或逻辑组件）：

| 字母 | 英文 | 问什么 |
|------|------|--------|
| **R** | Rate | 请求率多少？ |
| **E** | Errors | 错误率多少？ |
| **D** | Duration | 每次请求耗时分布？ |

**与 USE 关系：** RED 偏 **workload**（服务对外行为）；USE 偏 **resource**（底下资源）。网关、风控、策略服务可 RED；底层仍 USE。

---

### 工作负载特征分析（Workload Characterization）

用四个问题描述负载：

| 问题 | 内容 |
|------|------|
| **Who** | 谁产生负载（用户、市场、上游 feed） |
| **Why** | 为何此时产生（开盘、事件、策略触发） |
| **What** | 负载特征（读多写少、小包高频、burst） |
| **How** | 随时间如何变化（日周期、波动率 regime） |

**Gregg 强调：** 最大收益常来自 **消除不必要的工作** — 少算、少发、少 syscall、少拷贝，比「把必要的工作调快 5%」更划算。

---

### HFT 实战：业务链路分层埋点 + 工具串连

> **思路：** 沿 tick-to-trade **从外到内** 逐层加计数/计时；**先 Workload（埋点 / Grafana）确立业务真相**，再在数字对不上或异常已锁定时，用 **Resource 工具（tcpdump / perf / ss）** 交叉验证或下钻。  
> 与 [2.4 先上后下](./section-2.4-两种分析视角.md#先-workload后-resource--一句话) 一致 — 不是「只用命令」或「只用埋点」，是 **优先级**：counter / histogram **最先**，Linux 命令 **接力**。

**工具优先级（全链路通用）：**

```
P0  开发期埋点     counter（Rate）、histogram（P99）、段间 timer、错误码分类
P1  业务监控       Prometheus / Grafana  —  「tick 掉了多少」「P99 10→100 μs」
P2  交叉 / 下钻    tcpdump、ethtool、ss   —  埋点与网卡不一致，或 reject/P99 已锁定链路
P3  根因           perf -p <pid>、dmesg    —  锁定进程后看 CPU / 内核
```

**分层骨架：**

```
L2 解析入口         atomic counter（ticks/s）          ← P0 · 业务真相
L3 策略 / 报单      histogram + reject counter         ← P0
    ↓ 异常或 Rate 对不上
L0 网卡 ingress     tcpdump / ethtool -S               ← P2 · Resource 验证
L1 内核协议栈       ss -s、dmesg                       ← P2–P3
```
---

#### 第一步：每秒多少 tick？— 埋点为主，tcpdump 交叉

Workload 里 **RED 的 R** 在 HFT 行情侧就是 **ticks/s**。日常看 **Grafana / 解析 counter** 即可；怀疑 **feed 丢包、内核 drop、口径错误** 时，再用网卡层 **交叉验证**。

**路径 A · 解析入口（Workload · P0，主路径）**

在 **进入解析逻辑的第一行**（收完 buffer、尚未拆字段）用原子变量计数，export 到 Prometheus 或后台每秒读差分：

```cpp
std::atomic<uint64_t> g_parse_entry{0};

inline void on_market_buffer(const char* buf, size_t len) {
    g_parse_entry.fetch_add(1, std::memory_order_relaxed);
    // ... decode ...
}
// ticks/s = delta(counter) / Δt  →  Grafana panel
```

| 要点 | 说明 |
|------|------|
| **计数位置** | **解析入口**，不是策略 callback |
| **开发期必做** | 无此 counter，线上只有 `sar` pps，**不知道业务侧掉了多少 tick** |
| **口径** | 按 **包** 还是 **解码后 tick 条数** — dashboard 上写清楚 |

**路径 B · 网卡层（Resource · P2，验证 / 下钻）**

当 counter 显示 tick 掉速、或需确认「是交易所慢还是本地丢包」时，在接行情网口短抓包：

```bash
tcpdump -i eth0 -c 1000 -tttt --no-promisc 2>/dev/null | tee /tmp/ticks1000.log
# pps ≈ 999 / (t1000 - t1)
```

| 要点 | 说明 |
|------|------|
| **何时用** | counter 异常 · A≠B · 或 Workload 已锁定「parse 段」再查 NIC |
| **不是第一步** | 没人看 Grafana 就先 `tcpdump` — 不知道 **业务影响多大** |
| **过滤** | `udp port <feed>` — 与 counter 口径一致 |

**交叉验证 — 读差即下钻**（A = 解析 counter，B = tcpdump pps）

| 对比结果 | 含义 | 下一步 |
|----------|------|--------|
| **A ≈ B**（同口径） | 网卡→解析无大规模丢包 | Grafana 看 **L3 Duration / P99** |
| **A 掉、B 正常** | 包到了、解析没跟上，或 socket 丢 | `perf` 解析 PID · `ethtool -S` drop |
| **A > B** | 重复计数 / tcpdump 抓错口 | 核对绑核、DPDK bypass |
| **A、B 都低** | 交易所流速低 | 换 burst 窗口；勿闲时优化 μs |

**固定动作：**

```
1. Grafana / counter  →  ticks/s 是否异常？影响多大？
2. 异常时 tcpdump -c 1000  →  与 counter 交叉
3. 对齐 → 拆 P99 / reject；不对齐 → ethtool / perf
```
→ 命令索引：[2.2 Throughput 行](./section-2.2-术语与命令速查.md) · 排查顺序：[2.3.1](./section-2.3.1-时间尺度与排查走查.md)

**串连预览（Workload 确认 Rate 后的下一层）：**

| 层 | 问什么 | 工具（先 → 后） |
|----|--------|-----------------|
| L2→L3 | 解析→策略 **P99** | **histogram**（P0）→ 异常后 `perf` / bpftrace（P3） |
| L3→L4 | 报单 **reject**、ACK RTT | **Grafana 错误率**（P0）→ `tcpdump` 报单口 · `ss`（P2） |
| 全链路 | 哪段占 80% | 段间 histogram + 二分（见下） |

完整 reject 突增走查 → [2.4 报单示例](./section-2.4-两种分析视角.md#完整示例报单-reject-率突增)
---

### 下钻分析与五个为什么（Drill-Down & Five Whys）

**层次：**

```
监控（发现异常）
  → 识别（哪条路径 / 哪个资源）
    → 分析（为何如此）
      → 根因
```

**五个为什么：** 连续追问直至机制层（非止于「CPU 高」→ 要问「哪条线程、哪个函数、等谁」）。

---

### 延迟分析（Latency Analysis）

将**总延迟**拆成可度量子阶段，找出占比最大的段（可用**二分法**反复对半拆）。

**HFT 示例：**

```
tick 到达 → 解码 → 策略计算 → 风控 → 序列化 → 内核/网卡 → 交易所 ACK
   |___________|   |___________|   |___________________________|
        A                  B                    C
```

- 若 C 占 80% → 网络/内核/共置路径（Ch 10、DPDK、06 内核网络）
- 若 B 占 80% → 策略热点、锁、分配（Ch 6/7、perf 火焰图）

→ 与 Ch 1 **分布式追踪**、**火焰图** 直接衔接。

---

### 性能箴言（Performance Mantras）

调优优先级（从高到低）：

| # | 箴言 | 含义 |
|---|------|------|
| 1 | **别做** | 消除不必要工作 |
| 2 | **做，但别再做第二次** | 缓存 |
| 3 | **少做点** | 减工作量 |
| 4 | **晚点做** | 异步 / 批量写 |
| 5 | **趁别人没注意时做** | 避开高峰 |
| 6 | **并发做** | 并行化 |
| 7 | **用更便宜的方法做** | 换更低成本实现 |

HFT 热路径：**1 > 3 > 2 > 6** 最常见（少路径、少分配、热数据 cache、无锁/分片并行）；**4/5** 多用于日志、风控离线、非关键路径。

---


---

← [2.4 两种视角](./section-2.4-两种分析视角.md) · [2.2 命令速查](./section-2.2-术语与命令速查.md) · [本章导读](../README.md)
