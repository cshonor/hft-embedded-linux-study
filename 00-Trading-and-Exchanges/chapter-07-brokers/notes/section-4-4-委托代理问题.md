## 4. 委托代理问题 (Principal-Agent Problem)

核心经济学主题：**代理人 (经纪人) 利益 ≠ 委托人 (客户) 利益**。客户要经纪人 **努力且诚实**；经纪人可能 **偷懒** 或为 **自身利益** 牺牲客户。

### 4.1 最佳执行 (Best Execution)

| 要点 | 说明 |
|------|------|
| **义务** | 经纪人有义务为客户 **最佳执行** |
| **难题** | 概念 **难精确定量**；客户缺数据与专业知识 **审计执行质量** |
| **现实** | 多维度：价格、速度、成交概率、partial fill、市场冲击 |

| HFT 视角 |
|----------|
| 机构用 **TCA (Transaction Cost Analysis)**、implementation shortfall 量化 best ex |
| **Smart Order Router** 显式优化 price + fee + latency；监管要求 **订单处理披露 (606 reports 等)** |

### 4.2 双重交易 (Dual Trading Problem)

许多经纪人是 **经纪-做市商 (Broker-dealers)**：既 **代理客户** 又 **自营账户** 交易 → **严重利益冲突**。

典型冲突：把 **好价格** 留给自营，把 **差价格** 给客户。

| HFT 视角 |
|----------|
| **Trading ahead / information barrier**（Chinese wall）是合规重点 |
| 做市商 HFT 若同时服务外部 flow → **conflict policies** 与 **audit trail** 必须可 replay |

### 4.3 订单优先派送 (Order Preferencing)

经纪人常按做市商是否付 **PFOF** 决定 **路由**，而非纯 **最优价格** → 是否满足 best execution **长期争议**。

| HFT 视角 |
|----------|
| 与 [Ch 25 内部化](../chapter-25-internalization-preferencing-crossing/) 直接衔接 |
| **Internalization**、**payment for limit order flow** 是现代变体 |

---
