## 5.1 应用程序基础

### 设定目标与 Apdex

性能工作不是「感觉慢了就去调」— 需要**可测量的目标**：

| 目标类型 | 例子 | HFT 对应 |
|----------|------|----------|
| **延迟** | P50 / P99 / P999 | tick→信号、发单 RTT |
| **吞吐量** | 消息/秒、订单/秒 | 行情处理能力 |
| **资源利用率** | CPU%、网卡带宽 | 留 headroom 还是过载 |
| **成本** | 每百万笔 CPU 核时 | 云/共置 TCO |

**Apdex（Application Performance Index）：** 把响应时间映射到用户（或业务 SLA）体验：

```
Apdex = (满意数 + 可容忍数/2) / 总样本数
```

- **满意**：≤ T（阈值，如 1 ms）
- **可容忍**：T ~ 4T
- **受挫**：> 4T

HFT 里 Apdex 思路可借用：**把 tick 处理时间分桶**（<1 µs 满意、1–10 µs 可容忍、>10 µs 受挫），比只看平均值更能反映「偶发 GC / 锁竞争」对业务的伤害。

→ Ch 2 [统计与可视化](../../chapter-02-methodologies/) · P99 / 热力图

### 优化常见情况（Optimize the Common Case）

应用逻辑分支多，**Amdahl 定律**在这里很现实：

- 若 90% 时间在路径 A、10% 在路径 B → 把 B 优化 10 倍，整体只快 ~9%。
- **先 profile 生产或生产级 replay**，找占比最大的路径再动刀。

**HFT 常见「常见路径」：**

```
UDP/TCP 收包 → 解码 → 更新 order book → 策略计算 → 发单
         ↑                              ↑
    往往 Net I/O + Kernel          往往 User + Lock
```

→ [12-HFT Practice ch06](../../../21-hft-engineering/chapter-06-低延迟网络/) 端到端延迟分解

### 观测性与大 O 符号

**消除不必要的工作**是性价比最高的优化 — 但前提是**看得见**：

| 观测性层次 | 内容 | HFT |
|------------|------|-----|
| **Metrics** | QPS、延迟分位、队列深度 | Prometheus / 自建 counter |
| **Logs** | 结构化、可关联 trace id | 非热路径；热路径用 ring buffer |
| **Traces** | 跨阶段 span | tick 各阶段 timestamp |
| **Profiles** | CPU / off-CPU 栈 | perf、bpftrace |

**Big O：** 业务数据量涨 10 倍时，算法复杂度决定会不会「突然变慢」：

| 复杂度 | 数据量 ×10 时 | HFT 警示 |
|--------|---------------|----------|
| O(1) / O(log n) | 几乎不变 | order book 用合适结构 |
| O(n) | 线性变慢 | 全量扫描行情列表 |
| O(n²) | **灾难** | 嵌套循环配对、暴力撮合模拟 |

→ [01-CSAPP Ch3](../../../02-computer-systems/chapter-03-machine-level-programs/) 理解热点在汇编层的形态

---


### 常见陷阱

1. 应用层不剖析直接调内核——HFT 延迟 80% 在应用层（解码/策略），不先 profile 应用就调内核是本末倒置
2. CPI 高只查 CPU——CPI 高可能是 cache miss（查数据结构）、分支预测失败（查 if/switch），不只是 CPU 算力
3. 锁粒度不分层——全局锁 vs 分区锁 vs 无锁，不同竞争强度选不同方案，全局锁在高频路径是杀手

<details>
<summary>自测题（点击展开）</summary>

1. HFT 延迟优化的优先级是什么？
   <details><summary>答</summary>先应用层（消除不必要工作、数据结构优化）→ 再编译优化 → 再调度/绑核 → 最后内核/硬件</details>
2. CPI 高的可能原因有哪些？
   <details><summary>答</summary>cache miss（数据布局差）、分支预测失败（不可预测 if）、TLB miss（大页）、依赖链（ILP 不足）</details>
3. HFT 热路径锁优化方向？
   <details><summary>答</summary>全局锁 → 分区锁 → lock-free（CAS/RCU）→ 无共享（per-thread 数据）</details>

</details>


---

← [本章导读](../README.md)
