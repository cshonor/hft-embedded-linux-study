# Ch 15 大宗交易者 · Block Traders

> **Trading and Exchanges** · Larry Harris · **选读** · Part IV

本章探讨 **资金庞大的交易者** 如何安排 **无法通过常规机制轻松完成** 的超大额交易（**大宗交易 / Block trades**），以及专门为巨量订单提供流动性的 **「楼上市场」(Upstairs Market)**。

> **大宗交易 (Block)**：通常指 **超过日均交易量很大比例** 的订单 — 执行充满 **impact、信息泄露、信任** 挑战。

> **HFT 读者：** 与 **execution algos（VWAP/TWAP/IS）、dark pool、block crossing、IOI** 同谱系；[Ch 11 front-run](./chapter-11-指令预期者.md)、[Ch 10 stealth](./chapter-10-知情交易者与市场效率.md)、[Ch 5 经纪人市场](./chapter-05-市场结构.md) 衔接。

---

## 0. 本章主线

大宗发起人找流动性须克服 **四大难题** → 市场演化 **楼上机制 + 动机审计 + 与常规市场协调规则**。

---

## 1. 四大核心难题 (Block Trading Problems)

### 1.1 潜在需求问题 (Latent Demand Problem)

| 问题 | 许多愿交易者 **不在市场中** |
|------|---------------------------|
| **原因** | 管理限价单 **成本高**；或 **尚未意识到** 交易机会 |
| **名称** | **潜在需求 (Latent demand)** — 隐藏的 **响应型交易者 (Responsive traders)** |
| **任务** | 大宗方须 **主动发现** 这些隐藏对手 |

| HFT 视角 |
|----------|
| **Indication of Interest (IOI)**、**dark pool matching**、**broker capital introduction** |
| 电子时代：**conditional orders**、**pinging dark** — 仍是在挖 latent demand |

### 1.2 指令暴露问题 (Order Exposure Problem)

| 恐惧 | 广泛公开意图 → **「破坏市场」(Spoiling the market)** |
|------|------------------------------------------------------|
| **后果** | [Ch 11 抢先交易者](./chapter-11-指令预期者.md) **提前入场** |
| | **同向** 交易者 **加速** |
| | **反向** 交易者 **推迟** |
| **结果** | **价格冲击极大加速** |

| HFT 视角 |
|----------|
| **Stealth execution**、**iceberg**、**minimize footprint** — Ch 10 隐蔽交易的大单版 |
| **Flow sniffing / prediction** 是 exposure 的 **现代威胁** |

### 1.3 价格歧视问题 (Price Discrimination Problem)

| LP 恐惧 | 发起人 **隐瞒真实总规模**，**拆分吃流动性** |
|---------|---------------------------------------------|
| **手段** | 先用 **好价** 成交一部分，再用 **差价** 吃剩余 — **价格歧视** |
| **LP 要求** | 发起人须 **可信展示全部规模** |
| **匿名市场难点** | **撒谎无声誉成本** → 难信任 |

| HFT 视角 |
|----------|
| **Show full size or pre-negotiated block** vs **algo slicing** 张力 |
| **Last-look / firm-up** 机制 — 确认规模后报价 |

### 1.4 信息不对称问题 (Asymmetric Information Problem)

| LP 恐惧 | 发起人是掌握基本面的 **知情交易者 (Informed)** |
|---------|-----------------------------------------------|
| **后果** | 做对手盘 → **严重逆向选择**（[Ch 14](./chapter-14-买卖价差.md)） |

| HFT 视角 |
|----------|
| Block LP 要价含 **巨大 adverse selection premium** |
| **Buy-side TCA** — 证明 **utilitarian motive** 可降低 **冲击成本** |

---

## 2. 解决难题：须传递的可信信息

为获得流动性，大宗方须向 LP **可信地** 证明：

| 须证明 | 含义 |
|--------|------|
| **愿意价格让步** | 补偿 **库存风险 + 冲击** |
| **无隐藏额外规模** | 对抗 **价格歧视** 疑虑 |
| **尤其：不知情、效用型** | 对抗 **信息不对称** — 最难也最关键 |

---

## 3. 策略与机制

### 3.1 阳光交易 (Sunshine Trading)

| 做法 | **公开宣布** 身份、交易全貌、意图 |
|------|-----------------------------------|
| **有效条件** | 交易者 **广为人知** 且公认 **绝对不知情** |
| **例子** | **被动指数调整** 被迫再平衡 — 非 alpha 动机 |

| HFT 视角 |
|----------|
| **Index rebalance announcements** — 已知 utilitarian flow → **front-run 与提供 liquidity** 并存 |
| **ETF creation/redemption** 透明流程 — 部分 sunshine 逻辑 |

### 3.2 楼上市场 (The Upstairs Market)

大宗多通过 **电话** 在 **投资银行交易台** 完成 — 交易所大厅难做 **复杂信息交换**。

| 角色 | 职能 |
|------|------|
| **大宗交易做市商 (Block Dealers)** | **自有资金** 吃下客户大单 → 提供 **深度**；承担 **库存**；再 **慢慢平仓 (Lay off)** |
| **大宗交易经纪人 (Block Brokers)** | **大宗拼装者 (Block assemblers)** — 靠 **关系网 + 声誉** 寻找、撮合 **潜在需求** LP |

→ [Ch 7 经纪人](./chapter-07-经纪人.md) · [Ch 5 经纪人市场](./chapter-05-市场结构.md)

| HFT 视角 |
|----------|
| **Principal risk desk** vs **agency block desk** |
| **Capital commitment** — dealer 吃单后 **hedge/algo unwind** 是 HFT **impact 来源** |
| **Crossing networks**（[Ch 6](./chapter-06-指令驱动市场.md)、[Ch 25](./chapter-25-内部化优先撮合与交叉交易.md)）— 电子化楼上 |

### 3.3 交易动机审计 (Trading Motive Audit)

为打消 **信息不对称** 恐惧，楼上经纪/做市须 **审查客户动机**：

| 可信理由 | 易核实、**与基本面价值无关** |
|----------|------------------------------|
| **例子** | 清算组合筹资、缴遗产税、 **资产配置再平衡**（非单票 alpha） |

→ 确信 **不知情** 后才愿 **安排成交**

| HFT 视角 |
|----------|
| **Compliance questionnaire**、**restricted list**、**MNPI checks** |
| **Fund admin / corporate action** flow — 高 **utilitarian** 可信度 |

---

## 4. 统计现象：约 80% 大宗由卖方发起（美国股票）

| 理论解释 | 内容 |
|----------|------|
| **潜在需求 + 价格歧视** | 卖方受 **持仓数量**（或 **做空限制**）约束 → 更易证明 **「只能卖这么多」** → **全规模可信** |
| | 买方理论上可 **无限买入** 任意股票 → **规模难证** |
| **信息不对称** | 卖方易给 **非知情理由**（筹现金、调配置） |
| | 买方 **大举单票买入** 难让人相信 **非重大利好内幕** |

| HFT 视角 |
|----------|
| **Block sell** flow 常 **less toxic** than **aggressive block buy** — LP 定价不对称 |
| **Buyback / M&A** 例外 — 动机审计更严 |

---

## 5. 大宗市场与常规市场的协调

场外敲定后，须与 **公开 LOB** 利益协调：

### 5.1 打印 / 过户 (Print the Trade)

美国股市规则：大宗须 **带回交易所** **打印成交** — 保护 **公开限价单** 交易者利益（透明度、价格发现）。

### 5.2 规模优先与清理簿子 (Size Precedence · Clean Up the Book)

| 规则 | 例：超过 **25,000 股** 的大宗撮合可 **跳过时间优先** |
|------|------------------------------------------------------|
| **义务** | **清理限价指令簿 (Clean up the book)** — 须 **吃掉** 此时簿上所有 **价格更优** 的散户限价单 |

→ [Ch 6 规模优先 / 时间优先](./chapter-06-指令驱动市场.md)

| HFT 视角 |
|----------|
| **Trade-at rule**、**price improvement** 争议 — block print vs lit BBO |
| HFT **quote at BBO** 可能被 **block cross** **clean up** — **free option** 给散户 limit |

### 5.3 期货市场：期转现 (EFP)

**Exchange for Physical (EFP)** — 部分大宗在 **盘后** 以 **期货换现货** 形式结算。

---

## 6. 四大难题 × 机制对照

| 难题 | 机制回应 |
|------|----------|
| **潜在需求** | Block broker 关系网、IOI、dark matching |
| **指令暴露** | 楼上私下谈判、sunshine（特例）、algo stealth |
| **价格歧视** | 展示全规模、声誉、pre-negotiated block |
| **信息不对称** | **动机审计**、只信 utilitarian、卖方结构优势 |

---

## 7. 本章总结

| 要点 | 含义 |
|------|------|
| **Block ≠ 大市价单** | 是 **信任 + 信息 + 规模** 的 **合约设计问题** |
| **四大障碍** | Latent demand · Exposure · Price discrimination · Asymmetric info |
| **楼上市场** | Block dealer（吃库存）vs Block broker（拼装） |
| **动机审计** | 证明 **不知情效用型** 是成交 **关键** |
| **80% 卖方** | 规模可信 + 非知情理由 **不对称** |
| **与 LOB 协调** | Print + size precedence + **clean up book** |

> **HFT 读者 takeaway：** 你看到 **突然 block print / volume spike** — 可能是 **楼上 lay off** 的尾声，而非 **新发 initiator**；**index rebalance** 是 **sunshine + utilitarian** 的 **flow 日历**。Execution 系统要在 **Ch 10 stealth** 与 **Ch 15 动机可信** 之间选路径：**无审计的大暗单** → LP 要价 **极宽**。Part IV 流动性专题（Ch 13–15）至此收束；下一章 [Ch 16 价值交易者](./chapter-16-价值交易者.md) 回到 **知情投机** 分册。

---

## 相关章节

- 上一章：[chapter-14-买卖价差.md](./chapter-14-买卖价差.md)
- 下一章：[chapter-16-价值交易者.md](./chapter-16-价值交易者.md)
- 抢先与暴露：[chapter-11-指令预期者.md](./chapter-11-指令预期者.md)
- 隐蔽执行：[chapter-10-知情交易者与市场效率.md](./chapter-10-知情交易者与市场效率.md)
- 经纪人 / 楼上：[chapter-07-经纪人.md](./chapter-07-经纪人.md) · [chapter-05-市场结构.md](./chapter-05-市场结构.md)
- 交叉与内部化：[chapter-25-内部化优先撮合与交叉交易.md](./chapter-25-内部化优先撮合与交叉交易.md)
