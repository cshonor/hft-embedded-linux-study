# 第2章 交易系统关键组件

> **原书第 2 章 · The Critical Components of a Trading System**  
> **关键路径 · 网络 API · 非关键组件**

← 总览：[chapter-01](./chapter-01-高频交易基础与生态.md) · 引擎实现：[chapter-08](./chapter-08-超低延迟核心引擎开发.md)

---

## 本章定位

第二章回答：**如何把高频交易策略变成实时软件，并与交易所（或 ECN、暗池）连接、实际下单。**

| 划分 | 含义 |
|------|------|
| **关键路径（Critical Path）** | 行情进 → 决策 → 订单出；总延迟 = **Tick-to-Trade（T2T）** |
| **非关键路径** | 监控、日志、参数下发 — **不**参与 ns/μs 级下单决策，但 **系统可运维** |

---

## 1. 关键路径总览

```
External ──► Gateway IN ──► Book Builder ──► Strategy ──► OMS ──► Gateway OUT ──► Exchange
  (ECN/暗池)      │              │           Signal/Exec    风控      │
               协议解析        本地 LOB                         订单+回报
```

**T2T** = 上述链路 **纯处理时间**（常另加 **网络 RTT**）。HFT 优化 **每一跳**。

→ [chapter-10 测量 T2T](./chapter-10-延迟测量与基准压测.md)

---

## 2. 网关 (Gateways)

交易系统与 **外部流动性场所** 的桥梁。

### Gateway IN

| 职责 | 说明 |
|------|------|
| **收行情** | 从交易所网络接收 **market data** |
| **协议解析** | 外部格式（ITCH/FAST/SBE 等）→ **内部统一 Event struct** |
| **时间戳** | 尽早 **硬件打戳**（NIC PHC） |

### Gateway OUT

| 职责 | 说明 |
|------|------|
| **发订单** | New / Cancel / Replace |
| **收回报** | Ack / Fill / Reject — 驱动 **Execution** 与 OMS 状态机 |

**工程习惯：** IN / OUT **分进程或分核** — 避免收发包、解析 **争用** 同一线程。

→ [chapter-06 协议与传输](./chapter-06-低延迟网络与协议优化.md)

---

## 3. 订单簿构建器 (Book Builder)

**输入：** Gateway IN 的 add / modify / delete / trade。  
**输出：** 本地 **Limit Order Book（LOB）** — 策略 **零 RTT** 读 BBO/深度。

| 要求 | 说明 |
|------|------|
| **时间复杂度** | 插入/改/删 **O(1)** 或接近（按价 **索引**） |
| **数据结构** | **预分配 array/vector** 优于 **链表** — **cache locality** |
| **内存** | **Memory pool** 预分配 price level；热点 **无 malloc** |

### 与交易所 LOB 的关系

| | **交易所 LOB** | **本地 Book Builder** |
|---|----------------|----------------------|
| **权威** | 撮合 **真相源** | Feed **重建副本** |
| **策略读谁** | 不远程查 — **只读本地** | 必须 **与 feed 同步** |

→ 实现深化：[chapter-03 交易所动态与 LOB](./chapter-03-订单簿深度与行情解析.md)

---

## 4. 策略引擎 (Strategy)

系统 **大脑**，两子模块：

### Signal（信号）

- 消费 **Book** 更新
- 输出 **交易意图**（方向、价格、量、 urgency）

### Execution（执行）

- 处理 **市场反馈** — 如 **Reject（太慢/价格越界）** 后的 **改价/撤单/放弃**
- 与 OMS / Gateway OUT **协作**，非「发完即忘」

| 延迟要点 | 无虚函数热点路径 · 分支可预测 · 与 Book **无锁** 接口 |

→ [chapter-13 策略](./chapter-13-高频做市与套利策略.md) · [chapter-08 §7](./chapter-08-超低延迟核心引擎开发.md#7-关键路径组件应用层)

---

## 5. 订单管理系统 (OMS)

| 职责 | T2T 价值 |
|------|----------|
| **生命周期** | Created → Sent → Ack → PartialFill → Done / Cancelled / Rejected |
| **内部风控** | 量/价/频率/敞口 **违规 → 本地拒单** |
| **合规** | 不等交易所 **Reject RTT** 即可拦截 |

**原则：** 可疑订单 **不出 Gateway OUT** — 省 **往返延迟** 与 **交易所罚则风险**。

→ [chapter-11 风控合规](./chapter-11-风控合规与滑点控制.md)

---

## 6. 网络通信与 API

| 层 | 选择 |
|----|------|
| **API** | 交易所指定 **二进制会话 + 序列化格式** |
| **传输** | **TCP** — 可靠、有序（订单常 TCP） |
| | **UDP** — 不保证送达、**更低开销**（行情常 UDP 组播） |
| **物理** | 光纤 vs **微波/空芯** — 跨地域 **延迟套利** 基础设施 |

**系统设计关键考量：** 协议栈位置（内核 vs **kernel bypass**）、**同 NUMA 绑核**、**PTP 时钟**。

→ [chapter-05 OS](./chapter-05-操作系统内核极致调优.md) · [chapter-06](./chapter-06-低延迟网络与协议优化.md)

---

## 7. 非关键路径组件

不参与 **纳秒级下单决策**，但 **生产必备**：

| 组件 | 作用 |
|------|------|
| **Command & Control** | 交易员 **启停组件、改参数** — 与热点 **隔离** |
| **Position Server** | **汇总仓位**；OMS/Strategy **订阅** 防超限 |
| **Logging** | 异步 **环形缓冲** 落盘 — **不在热点 sync 写** |
| **Viewers** | **只读** 监控订单/成交/告警 |

```
        ┌── Command & Control
        │
Hot ────┼── Gateway…Strategy…OMS  ──── Logging (async)
 path   │
        └── Position Server ◄── subscribe ── OMS / Strategy

        Viewers (read-only monitor)
```

→ [chapter-12 实盘运维](./chapter-12-实盘上线与运维进阶.md) · [chapter-07 无锁日志环](./chapter-07-无锁数据结构与内存布局.md)

---

## 8. 撮合原理（策略视角·补充）

| 概念 | HFT 含义 |
|------|----------|
| **Price-time priority** | 同价 **先到先得** — queue position |
| **BBO** | Signal 最常消费 |
| **Latency arb** | 更快 **看见/行动** BBO 变化 |

深入理论 → [00-Trading-and-Exchanges](../00-Trading-and-Exchanges/)

---

## 本章小结

| 关键路径 | Gateway → Book → Strategy → OMS → Gateway |
|----------|---------------------------------------------|
| **Book** | **Vector/预分配** · **O(1)** · 本地 LOB |
| **Strategy** | **Signal + Execution** |
| **OMS** | **内部风控** 前置 |
| **非关键** | C&C · Position · Log · Viewers |

**下一步：** [chapter-03 订单簿实现](./chapter-03-订单簿深度与行情解析.md) · [chapter-08 C++ 引擎规范](./chapter-08-超低延迟核心引擎开发.md)
