# Ch 3 §2 交易促成机构 · Trade Facilitators

> **Trading and Exchanges** · Larry Harris · **精读**

## 2. 交易促成机构 (Trade Facilitators)

**交易促成机构** — 提供 **交易场地、规则与技术** 的 **基础设施方**：

| | |
|---|---|
| **做什么** | 把 **做市商买卖价**、**买方委托单** 集中起来，按 **公开规则** 自动 **报价–撮合–成交** |
| **不做什么** | **不** 用自有资金与客户对赌（那是 **做市商/Dealer**，见 [§1](./section-1-1-市场参与者-买方与卖方.md)） |
| **目标** | 让交易 **高效、公平、可预期** 地发生 |

→ 你写的 go-dex **就是** 最小化的「交易促成机构」：**OrderBook + `Match()` + 规则**。

---

## 2.1 交易所 (Exchanges) — 最核心的促成平台

**Nasdaq、NYSE** 等 **上市股票交易所** 是最核心的 **公开「报价–撮合」平台**：

```
做市商 / 散户 / 机构的限价单、报价
              │
              ▼
    ┌─────────────────────┐
    │  集中订单簿 (LOB)    │  ← 透明、按规则排队
    │  价格–时间优先撮合    │
    └─────────────────────┘
              │
              ▼
           成交回报
```

| 特征 | 说明 |
|------|------|
| **公开透明** | 最优买卖价 (BBO)、（专业用户）Level II 深度 |
| **规则驱动** | 撮合优先级、订单类型、涨跌停 — [Ch 4](../chapter-04-orders-and-order-types/) · [Ch 6](../chapter-06-order-driven-markets/) |
| **历史形态** | NYSE **物理大厅 + 专家**；现代 **全电子**（SuperDot / SuperSOES → 今日 matching engine） |

**Harris 故事入口：** [Ch 2 §1 散户 Jennifer](../../chapter-02-trading-stories/notes/section-1-1-散户股票交易.md) — 单进 **公开簿** 的典型路径。

**HFT：** **Colocation** 在 **exchange matching engine** 旁；竞争 **queue position**、**ingress 顺序**。

---

## 2.2 ECN（电子通信网络）

**ECN (Electronic Communication Network)** — **电子化** 的交易促成平台，常与 **传统交易所并列** 或 **嵌入** 路由链：

| 角色（实务表述） | 说明 |
|------------------|------|
| **连接参与者** | 买方、卖方、做市商 **直接** 在电子网络上挂撤单 |
| **聚合 / 接力流动性** | 当 **第一层做市商** 接不住单时，路由 **继续找** ECN 等 **更优报价** — 见 [Ch 2 §1 Level II 多层接力](../../chapter-02-trading-stories/notes/section-1-1-散户股票交易.md) |
| **第四市场** | 与 [§4 市场分布](./section-4-4-交易市场的分布.md) **primary + fourth market** 并列 |

```
订单路由（简化）
  做市商内部化 / 义务报价
       │ 接不住
       ▼
  ECN 更优价
       │ 仍不够
       ▼
  其他 venue / 交易所 / …
```

**与 SOR 的关系：** **ECN** 是 **liquidity venue** 的一种；**SOR (Smart Order Router)** 是 **跨多个 venue（含 ECN、交易所）找最优价** 的 **软件层** — HFT / 机构 **在促成机构之上** 再叠一层。

**HFT：** 同时在 **primary + ECN** 挂单/吃单 → **跨 venue 套利**；**Reg NMS** 下 **best execution** 必须 **扫多个促成平台**。

---

## 2.3 暗池 (Dark Pools)

**暗池** — **机构间** 的 **替代交易促成平台 (ATS)**，核心卖点是 **隐藏**：

| | 公开交易所 | 暗池 |
|---|-----------|------|
| **报价可见性** | BBO / Level II **公开** | **不显示** 挂单（成交前） |
| **典型用户** | 散户 + 机构 **小额** | **大额** 机构块 |
| **主要目的** | 价格发现 + 流动性 | **降低市场冲击 (impact)**、防 **信息泄露** |

**Harris 故事：** [Ch 2 §2 Bob → POSIT](../../chapter-02-trading-stories/notes/section-2-2-机构股票交易.md) — 大单 **先进暗池** 保密撮合，再考虑公开簿。

```
机构 100 万股卖单
  → 若直接扫公开 Asks：所有人看见 → 抢跑、砸盘、冲击成本 ↑
  → 暗池：与对手 **匿名交叉** — 价格常近 **中间价**，量 **不暴露** 在 Level II
```

**注意：** 暗池 **也是** 交易促成机构 — 提供 **场地与撮合规则**，只是 **信息披露规则** 与公开所不同。不是「场外乱谈价」；仍受 **ATS 监管**（如美国 SEC Reg ATS）。

---

## 三类促成平台对照

| 类型 | 代表 | 透明度 | 典型场景 |
|------|------|--------|----------|
| **交易所** | NYSE、Nasdaq | **高** | 散户、做市、价格发现、HFT 主战场 |
| **ECN** | 历史上 INET 等 | **高**（电子簿） | 电子化撮合、与所竞争、路由接力 |
| **暗池** | POSIT、银行 ATS | **低**（成交前隐藏） | 机构 **大块**、减 impact |

**共同本质：** **基础设施** — 定规则、跑撮合、出成交回报；**盈亏在参与者之间**，不在「交易所本身做庄」。

---

## 与 go-dex 的对照

| 真实世界 | go-dex 现状 |
|----------|-------------|
| 交易所 **公开 LOB** | 单一 `OrderBook` + `Match()` |
| ECN / 多 venue **SOR** | **未实现** — M5+ 在 `Match()` 外叠 **路由层** |
| 暗池 **隐藏簿** | **未实现** — M5+ 第二簿或 cross 模块 |

→ [go-dex OUTLINE](../../00-practice-go-dex/OUTLINE.md) · [Ch 2 §2 机构模块清单](../../chapter-02-trading-stories/notes/section-2-2-机构股票交易.md)

---

## 2.4 清算与结算代理 (Clearing and Settlement Agents)

促成 **成交之后** 的 **交付链** — 与「撮合」分工：

| 环节 | 职能 | 美国示例 |
|------|------|----------|
| **清算 (Clearing)** | 匹配买卖记录，确认条款一致 | NSCC |
| **结算 (Settlement)** | 资金与证券最终交割 | 证券常 **T+1/T+2 净额结算** |
| **清算所 (Clearinghouses)** | 衍生品：**CCP** 担保 + **保证金** | CME Clearing, OCC |

→ HFT：**pre-trade risk** 在发单前；**post-trade** 由 clearing 承接；保证金约束策略容量

---

## 2.5 存管与托管 (Depositories and Custodians)

代客持有现金与证券凭证，协助结算快速完成交割。

- 例：**DTC**（Depository Trust Company）

→ HFT：热路径不经过 DTC，但 **实盘对接** 必须理解 settlement 链路

---

← [§1 参与者](./section-1-1-市场参与者-买方与卖方.md) · [Ch 3 README](../README.md) · 下一节 [§3 交易工具](./section-3-3-交易工具.md)

**交叉：** [Ch 1 §3 交易工具与市场](../../chapter-01-introduction-market-microstructure/notes/section-3-3-交易工具与市场.md) · [Ch 5 市场结构](../../chapter-05-market-structures/)
