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
