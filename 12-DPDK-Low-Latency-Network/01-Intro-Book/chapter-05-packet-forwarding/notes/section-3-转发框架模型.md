## 3. 两大转发框架模型

> 思想源自专用 **网络处理器 (NP)**；DPDK 将其移植到 **通用 IA 多核**

---

### 一、Run to Completion (运行至终结)

| 特点 | 说明 |
|------|------|
| **一核一线** | 每个 lcore 负责报文 **完整生命周期**（RX → 处理 → TX） |
| **绑定** | EAL `-c` / `-l` **绑核** |
| **优点** | 编程 **简单**、**横向扩展** 直观（加核加吞吐） |
| **缺点** | 单包逻辑 **耦合** 在一核；难对 **单一阶段** 做专用优化 |

**HFT 常见：** 行情 tick 路径 **单核 RTC** — 最小跨核、最小队列。

→ [Ch8 Run-to-Completion 结合](../chapter-08-flow-classification-multiqueue/notes/section-2-网卡多队列.md)

---

### 二、Pipeline (流水线 / Packet Framework)

借鉴 **工业流水线**：处理拆成多个 **逻辑功能单元**，分布在同核或不同核，**队列传递** mbuf。

**三大要素：**

| 要素 | 角色 |
|------|------|
| **Port（逻辑端口）** | 报文进出 Pipeline 的抽象端点 |
| **Table（查找表）** | 匹配 — Hash / LPM / ACL 等 |
| **Action（处理逻辑）** | 命中后的修改、转发、丢弃 |

- 可用 **脚本化配置** 快速搭 **自定义网络产品**（交换机、防火墙逻辑等）
- Stage 间通常经 **rte_ring** — [Ch4 无锁 ring](../chapter-04-synchronization/notes/section-5-无锁机制.md)

---

### 三、选型对照

| | **Run to Completion** | **Pipeline** |
|---|----------------------|--------------|
| 延迟 | 通常 **更低**（无 stage 排队） | 多 stage 可能增加 **排队延迟** |
| 吞吐扩展 | 加 **完整 pipeline 副本**（多核各跑全流程） | 对 **热点 stage** 单独 **水平扩展** |
| 代码复杂度 | 低 | 高 — 需定义 Port/Table/Action |
| 典型产品 | 简单 L3 转发、低延迟网关 | 复杂分类、多表 lookup 产品 |

```
简单 + 低延迟     →  RTC + RSS（Ch8）
复杂 Match/Action →  Pipeline + rte_table_*
```

---

← [2. 处理模块划分](./section-2-网络处理模块划分.md) · 下一节 [4. 转发算法](./section-4-核心转发算法.md)
