# Ch 3 交易产业 · The Trading Industry

> **Trading and Exchanges** · Larry Harris · **精读**

本章对交易行业做 **全景式概述**：参与者、促成机构、交易工具、市场分布、监管体系——建立理解后续 LOB / 订单 / 规则的 **产业地图**。

---

## 1. 市场参与者：买方与卖方 (The Players)

交易产业分为 **买方（购买流动性服务）** 与 **卖方（提供流动性服务）** 两大阵营。

### 1.1 买方 (The Buy Side)

利用市场解决财务或风险问题的个人、基金、企业、政府：

| 类型 | 动机 | HFT 视角 |
|------|------|----------|
| **投资者 / 借款人** | 财富跨期转移 | 通常不是 HFT 对手方主类型 |
| **套期保值者 (Hedgers)** | 降低/转移商业风险 | 期货/FX HFT 的客户流之一 |
| **资产交换者** | 换持有更合适的资产 | 机构 rebalance → 大单 impact |
| **赌徒 (Gamblers)** | 娱乐 | 零售流；payment for order flow 来源 |

### 1.2 卖方 (The Sell Side)

帮助买方完成交易的专业机构或个人：

| 类型 | 角色 | HFT 视角 |
|------|------|----------|
| **做市商 (Dealers)** | 自有资金与客户交易，低买高卖赚价差 | **HFT 做市** 的直接对标；流动性供给方 |
| **经纪人 (Brokers)** | 代理人，撮合对手、赚佣金 | DMA / 算法单路由的上游 |
| **经纪-做市商 (Broker-dealers / Wirehouses)** | 做市 + 经纪一体 | 大投行；内部化订单流 (internalization) |

> **HFT 定位：** 卖方中的 **电子化做市商 / 流动性提供者**；与 buy-side institution 的 **执行算法** 在同一生态博弈。

---

## 2. 交易促成机构 (Trade Facilitators)

### 2.1 交易所 (Exchanges)

提供交易者会面、安排交易的场所：

- 传统：**物理大厅**
- 现代：**计算机化指令驱动系统**、**ECN**

并非所有工具都在交易所交易——债券等主要在 **OTC 场外**。

| HFT 关联 |
|----------|
| 多 venue 竞争 → 跨交易所套利、订单路由 |
| Colocation 在 **exchange matching engine** 旁 |
| ECN = **第四市场** 组成部分 |

### 2.2 清算与结算代理 (Clearing and Settlement Agents)

| 环节 | 职能 | 美国示例 |
|------|------|----------|
| **清算 (Clearing)** | 匹配买卖记录，确认条款一致 | NSCC |
| **结算 (Settlement)** | 资金与证券最终交割 | 证券常 **T+3 净额结算** |
| **清算所 (Clearinghouses)** | 衍生品：**中央对手方 (CCP)** 担保 + **保证金** | CME Clearing, OCC |

→ HFT：**pre-trade risk** 在发单前；**post-trade** 由 clearing 承接；保证金约束策略容量

### 2.3 存管与托管 (Depositories and Custodians)

代客持有现金与证券凭证，协助结算快速完成交割。

- 例：**DTC**（Depository Trust Company）——全球最大存管机构之一

→ HFT：热路径不经过 DTC，但 **实盘对接** 必须理解 settlement 链路

---

## 3. 交易工具 (Trading Instruments)

| 类别 | 内容 | HFT 主战场 |
|------|------|-----------|
| **实物资产** | 现货商品、房地产、排污权等 | 部分 commodity |
| **金融资产** | 股票、债券等现金流所有权 | **Equities** 核心 |
| **衍生品** | 远期、期货、期权、掉期；通常 **零净供给** | **Futures / Options** 核心 |
| **保险与博彩合约** | 取决于不确定事件 | 非典型 HFT |
| **混合工具** | 可转债、认股权证等 | 专项 arb |

**零净供给 (Zero net supply)：** 衍生品多头与空头头寸汇总为零——每多一仓必有一空仓，HFT 常在 **做市** 中同时挂双边。

---

## 4. 交易市场的分布 (Where are the Trading Markets?)

### 4.1 股票市场

| 层级 | 说明 |
|------|------|
| 主要上市市场 | NYSE、Nasdaq |
| 区域性交易所 | 历史遗留，重要性下降 |
| **第三市场 (Third market)** | 上市股票在 **场外做市商** 处交易 |
| **第四市场 (Fourth market)** | **ECN / ATS** 等替代交易系统 |

→ HFT：同时在 primary + ECN + dark pool 路由；**Reg NMS** 下 best execution 跨 venue

### 4.2 期权与期货市场

- 专门交易所：CBOE、CBOT/CME 等
- 与股票不同：期货/期权 **通常不在多 venue 交易同一合约**
- 各所有独立清算所；通过 **设计新合约** 竞争

→ HFT：期货 **单 venue 单合约** → latency 竞争更集中

### 4.3 债券与外汇市场

- 企业债、市政债、**现货 FX**：绝大多数 **OTC**
- 投资银行 / 商业银行报价驱动

→ HFT：FX **电子化程度高**（EBS、Reuters）；公司债 HFT  niche

---

## 5. 市场监管 (Market Regulation)

### 5.1 政府监管机构（美国）

| 机构 | 管辖 |
|------|------|
| **SEC** | 证券、股票期权 |
| **CFTC** | 商品、期货 |
| **Fed (Regulation T)** | 投机性 **保证金** 要求 |

→ HFT：SEC/CFTC 对 **market access rule、熔断、裸单禁止** 等直接约束系统

### 5.2 自律组织 (SROs)

交易所、清算所、**NASD (Finra 前身)** 等：

- 制定会员细则
- 违规：罚款、开除

→ HFT：exchange **会员资格**、market maker 义务、order type 规则由 SRO 层面细化

---

## 本章总结

| 维度 | 核心 takeaway |
|------|---------------|
| **参与者** | Buy-side 要流动性；Sell-side 供流动性——HFT 在卖方 |
| **基础设施** | Exchange → Clearing → Settlement → Depository 全链路 |
| **工具** | Equity / Futures / Options 是 HFT 主战场 |
| **市场结构** | 股票多 venue；期货单 venue；债券/FX 偏 OTC |
| **监管** | SEC/CFTC + SRO 双层 |

> **HFT 读者 takeaway：** 第三章是 **产业架构图**——设计交易系统时，需明确：你是哪类 participant、走哪条 venue、触哪层 clearing、受哪条 regulation。后续 LOB/订单章在此框架下展开。

---

## 相关章节

- 上一章：[chapter-02-交易故事.md](./chapter-02-交易故事.md)
- 下一章：[chapter-04-交易指令与订单类型.md](./chapter-04-交易指令与订单类型.md)
