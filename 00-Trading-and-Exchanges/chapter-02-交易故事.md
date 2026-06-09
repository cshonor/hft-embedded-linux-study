# Ch 2 交易故事 · Trading Stories

> **Trading and Exchanges** · Larry Harris · **精读**

本章通过七个日常交易案例，展示股票、债券、期货、外汇市场中各类参与者 **如何实际安排并完成交易**。制度细节密集，目的是让读者直观感受交易中要解决的实际问题，为全书微观结构分析奠基。

---

## 1. 散户股票交易 (Retail Equity Trades)

### NYSE 上市股票

散户 Jennifer 购买 200 股 AT&T：

1. 经纪人查看 **综合报价系统**（最优买卖价 BBO）
2. 可选 **市价单 (Market order)** 或 **限价单 (Limit order)**
3. 订单经 **SuperDot** 路由至 NYSE 大厅 **专家 (Specialist / 做市商)**
4. 在 **电子限价指令簿** 中等待撮合，或由专家自行促成

| 概念 | HFT 关联 |
|------|----------|
| BBO / 限价簿 | 现代 LOB 的原型；HFT 竞争 queue position |
| Specialist | 演化 today's designated market maker (DMM) |
| SuperDot | 早期电子路由 →  today's direct market access |

### Nasdaq 股票

Jennifer 购买微软 (Nasdaq)：

1. 查看 **Nasdaq Level II**——多做市商 + ECN 分层报价
2. 订单路由至特定做市商（可能含 **订单流返点 payment for order flow**）
3. 或通过 **SuperSOES** 等自动执行系统成交

| 概念 | HFT 关联 |
|------|----------|
| Level II 深度 | 订单簿多档报价；HFT 读 full book |
| ECN | 与交易所竞争的电子化 venue |
| 订单流返点 | 零售流 vs 机构流；HFT 通常走 DMA 不经此路径 |

---

## 2. 机构股票交易 (Institutional Equity Trades)

### 流动性好的大盘股

机构交易员 Bob 为养老基金买入 **40 万股 ExxonMobil**（约 $1600 万）：

1. 先用 **POSIT**（机构暗池）以 **价差中间价** 保密撮合部分数量——降低 **市场冲击 (market impact)**
2. 剩余通过 **大宗经纪 / 场内经纪** 分批完成，**不暴露全部意图**

→ HFT：与 institution 相反——HFT 拆单极小、速度极快；institution 怕 impact，HFT 有时 **制造** impact 或 **捕捉** impact 后的 revert

### 流动性差的 Nasdaq 股票

Bob 卖出 10,000 股 **价差极大、流动性极差** 的股票：

- 直接电话联络该券 **大型做市商**
- **同时询问买价和卖价（双向报价）**——防止做市商猜出卖意而 **恶意压价**

→ HFT：信息不对称博弈；never reveal direction when negotiating size

---

## 3. 超大宗股票抛售 (A Very Large Block Stock Trade)

Edna 须抛售 **90 万股** 家族企业股票（数千万美元）缴遗产税：

1. 高盛 **大宗经纪 (block broker)** 接手
2. **不能** 直接在公开市场抛售（会 **价格暴跌**）
3. 查 **13F 公开持仓**，定位对该股有兴趣的大型基金
4. 场外 **Upstairs market** 成交：提供 **价格折扣 (discount)** + 说明抛售原因是 **纳税而非基本面恶化**

| 概念 | HFT 关联 |
|------|----------|
| Block / upstairs | 与 HFT 热路径分离的大宗协商市场 |
| Information leakage | 大单意图暴露 = 最大敌人 |
| 13F | 公开信息用于找 counterparty——HFT 不碰此类但需知 institution 行为 |

---

## 4. 期货市场：套期保值 (A Futures Market Trade)

大豆加工商在 **CBOT** 套期保值 (Hedging)：

- **场内 Pit**：喊价 (Shouting) + 手势（手心向外=卖，向内=买）
- 清算：**BOTCC** 作中央对手方，担保履约 + 管理 **保证金**

→ HFT：期货是 HFT 主战场之一；Pit 已被 **全电子 CME Globex** 取代，但 **中央清算 + 保证金** 机制不变

---

## 5. 期权市场交易 (An Options Market Trade)

Lisa 持有暴涨微软股票，买 **看跌期权 (Put)** 避税 + 防跌：

1. **适合性测试 (Suitability requirements)**——期权高风险
2. 限价单 → 太平洋交易所期权大厅
3. **OBO (订单簿官员)** + 场内做市商撮合
4. **OCC** 清算担保

→ HFT：期权 market making 是独立 HFT 子领域（vol surface、delta hedge）；与 equity LOB HFT 不同 skill set

---

## 6. 债券市场交易 (A Bond Market Trade)

Sam 购买 **$5000 万** 长期公司债：

- **OTC 场外**，非集中交易所
- 电话 **所罗门兄弟销售交易员**，以 **10 年期国债收益率** 为基准讨价还价
- **NSCC** 电子记账，**货银对付 (DVP)**

→ HFT：公司债 HFT 较少；**国债 / 利率 futures** 更相关；OTC = 议价而非 LOB 竞价

---

## 7. 外汇市场交易 (A Foreign Exchange Trade)

美国制造企业 CFO 购买 **500 万英镑**：

- **银行间电子网络**（全球最大市场之一）
- 本地银行 → Reuters 终端 → 纽约/全球大行 **双向报价** → 快速成交

→ HFT：FX 是 core HFT asset class；**EBS / Reuters Matching** 演进为 electronic LOB；latency arb 跨 venue 常见

---

## 本章总结：七个故事 → 全书概念地图

| 故事暴露的概念 | 全书后续章节 |
|---------------|-------------|
| **买卖价差 (Bid-ask spread)** | 流动性、做市成本 |
| **市场冲击 (Market impact)** | 机构执行、HFT 与 institution 互动 |
| **大宗隐藏 vs 暴露** | 暗池、information leakage |
| **信息不对称** | 知情 vs 不知情、逆向选择 |
| **做市商 / 专家角色** | LOB、queue priority |
| **电子系统 vs 人工大厅** | 市场结构演变、colocation |
| **清算结算机制** | 信用风险、保证金、DVP |

> **HFT 读者 takeaway：** 第二章是「显微镜下的真实交易」——每个故事对应你将来系统里的一条路径：路由 (routing)、订单类型 (order type)、venue 选择、impact 控制、清算对接。读技术书 (Rosen/DPDK) 之前，先能在业务语言里 **讲清楚一笔单怎么走完**。

---

## 相关章节

- 上一章：[chapter-01-引言与市场微观结构.md](./chapter-01-引言与市场微观结构.md)
- 下一章：[chapter-03-订单类型与LOB.md](./chapter-03-订单类型与LOB.md)
