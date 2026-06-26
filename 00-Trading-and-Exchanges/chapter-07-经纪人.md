# Ch 7 经纪人 · Brokers

> **Trading and Exchanges** · Larry Harris · **选读** · Part I

本章全面探讨 **经纪人在交易产业中的角色**、经纪公司 **内部架构**、**盈利模式**，以及委托人与经纪人之间不可避免的 **委托代理问题** 与潜在 **欺诈行为**。

> **HFT 读者定位：** 直连 (DMA) / co-location 绕过了「人工经纪人」，但 **清算结算、会员资格、路由与 best execution 义务** 仍在；PFOF、internalization 是本章经济学在现代的延续 → 见 [Ch 25](./chapter-25-内部化优先撮合与交叉交易.md)。

---

## 1. 经纪人的核心定义与作用 (What Brokers Do)

经纪人是 **代表客户安排交易的代理人 (Agents)**。与用 **自有资金** 交易的做市商不同，经纪人交易的是 **客户订单**，通过 **服务** 赚取 **佣金 (Commissions)**。

交易者使用经纪人的主要原因：

| 原因 | 要点 |
|------|------|
| **清算与结算** | 不愿与陌生对手方裸奔交易；经纪人 **担保交割**，以较低成本解决 **信任与信用** |
| **市场准入** | 交易所通常仅 **会员** 可进；公众须通过 **会员经纪人** 或 **引入经纪人 (Introducing broker)** |
| **专业交易技能** | 尤其 **大宗经纪人**：知道谁有意愿、擅长谈判、会做 **订单敞口管理 (Order Exposure Management)**——在 **不造成巨大市场冲击** 下隐蔽完成大单 |
| **代理限价单** | 代客户 **监控市场** 并执行限价单、止损单（客户无法盯盘时） |

| HFT 视角 |
|----------|
| **DMA / sponsored access**：算法直连交易所，但 **legal counterparty** 仍是 broker-dealer |
| **Execution algos / SOR** = 程序化版的 exposure management；**impact / footprint** 仍是核心指标 |
| Co-located HFT 自己管订单，但 **clearing、margin、credit** 仍走 broker 后台 |

---

## 2. 经纪公司内部架构 (Structure of a Brokerage Firm)

大型经纪公司通常分三块：

### 2.1 前台 (Front Office)

直接面向客户：

| 部门 | 职能 |
|------|------|
| **销售与交易 (Sales & Trading)** | 拉订单、执行、客户关系 |
| **企业融资 (Investment Banking)** | 协助 **发行证券** |
| **研究 (Research)** | 投资分析与报告 |

### 2.2 后台 (Back Office)

支撑交易运转：

| 部门 | 职能 |
|------|------|
| 账户维护 | 持仓、成本、报表 |
| **清算与结算 (Clearing & Settlement)** | 与 DTCC/托管行对接 |
| 信息系统 | OMS、风控、报表 |
| **信用管理 (Credit management)** | 保证金、授信 |
| **合规 (Compliance)** | 监管报送、best execution 审计 |

### 2.3 自营 (Proprietary Operations)

| 业务 | 说明 |
|------|------|
| **现金管理** | 客户闲置资金的投资 |
| **融券 (Securities lending)** | 借出证券给 **做空者** |
| **风险管理** | 公司级风险敞口 |

| HFT 视角 |
|----------|
| HFT  firm's **tech stack** 多在前台 execution；**back office latency** 影响 end-of-day margin、locate |
| **Prime brokerage** 把 credit、securities lending、cross-margin 打包——机构 HFT 常见模式 |
| 合规与 **CAT / MiFID II best ex reporting** 是 post-trade 的硬约束 |

---

## 3. 利润来源 (Revenues)

收入来源 **远不止佣金**：

| 来源 | 说明 |
|------|------|
| **佣金 (Commissions)** | 主收入；全服务 vs 折扣 broker 费率差大；大客户可 **议价** |
| **订单流付款 (Payments for Order Flow, PFOF)** | 做市商为获得经纪人客户的 **市价单** 而支付的 **回扣** |
| **利息 (Interest)** | **保证金贷款** 利差；客户 **现金余额** 再投资 |
| **卖空利息 (Short interest rebate)** | 做空所得资金赚利息；通常 **不全额返还** 散户 |
| **其他** | 承销费、并购咨询、 **证券出借费** |

| HFT 视角 |
|----------|
| **PFOF** → 零售订单路由至 internalizer / wholesaler；与 **price improvement** 争议（Reg NMS best ex） |
| HFT **maker rebate / taker fee** 是 **交易所** 侧，不是 broker 佣金——但 economic bundle 类似「谁付谁订单流」 |
| **Stock loan / locate** 成本直接影响 **short-selling HFT** 策略可行性 |

---

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
| 与 [Ch 25 内部化](./chapter-25-内部化优先撮合与交叉交易.md) 直接衔接 |
| **Internalization**、**payment for limit order flow** 是现代变体 |

---

## 5. 经纪人欺诈与违规 (Broker Fraud)

极少数不诚实经纪人利用 **信息优势** 与 **客户无知** 欺诈。典型手段：

| 违规 | 英文 | 手段 |
|------|------|------|
| **抢先交易** | Front running | 执行客户大单（将 **推动价格**）前， **自营同向** 建仓牟利 |
| **欺诈性交易分配** | Fraudulent trade assignment | **优价成交** 分给偏爱客户或自营； **劣价** 分给其他人 |
| **预先安排交易与回扣** | Prearranged trading & kickbacks | 未公开竞价，与勾结做市商 **劣价成交** 客户单，换 **私下回扣** |
| **过度交易/刷单** | Churning | 为赚佣金 **频繁无意义** 买卖（"churn 'em and burn 'em"） |
| **资产挪用** | — | 极端情况 **盗窃** 客户资产 |

### 破产保障

美国 **证券投资者保护公司 (SIPC)** 为投资者现金与证券提供 **最高 50 万美元** 保险（防范 **经纪公司破产** 风险）。

| HFT 视角 |
|----------|
| **Front running** 在算法时代 → **latency arbitrage on customer flow**、**information leakage** 监管案例 |
| **Allocation fairness** → 多策略基金内的 **trade allocation**、avg price 分配 |
| 选 prime broker 时 **capital、credit rating、segregation of assets** 是 **operational risk** 核心 |

---

## 6. 本章总结

| 维度 |  takeaway |
|------|-----------|
| **经纪人 = 代理人** | 赚 **服务与利差**，不是 principal 风险（除非 broker-dealer 自营） |
| **价值链** | 前台拉单 → 后台清算信用 → 自营与借贷 **交叉补贴** 低佣金 |
| **经济学** | 委托代理 → best ex 难证、dual trading、PFOF / preferencing |
| **风险** | 欺诈谱系从 churning 到 front running；SIPC 是 **最后防线** |

```
客户订单
    → 经纪人（代理 + 可能自营）
    → 路由 / 执行 / 清算
    → 佣金 + PFOF + 利息 + 出借 ＝ 收入
    → 利益冲突贯穿全程
```

> **HFT 读者 takeaway：** 即使你 **co-locate 直连**，仍在 **broker-dealer 生态** 里：margin、locate、prime、best ex、PFOF 的 **二阶效应** 影响 **订单流质量** 与 **监管边界**。本章是理解 **为什么 retail flow 和 institutional flow 被 differentially routed** 的业务基础。

---

## 相关章节

- 上一章：[chapter-06-指令驱动市场.md](./chapter-06-指令驱动市场.md)
- 下一章：[chapter-08-为什么人们要交易.md](./chapter-08-为什么人们要交易.md)
- 产业背景：[chapter-03-交易产业.md](./chapter-03-交易产业.md)
- 内部化与路由：[chapter-25-内部化优先撮合与交叉交易.md](./chapter-25-内部化优先撮合与交叉交易.md)
