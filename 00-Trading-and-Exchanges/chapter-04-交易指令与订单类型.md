# Ch 4 交易指令与订单类型 · Orders

> **Trading and Exchanges** · Larry Harris · **精读**

**指令 (Orders)** 是交易者无法亲自协商时，向经纪人或交易所传达的 **交易意愿与条件**——构建一切策略的积木。理解指令 = 理解 **流动性从哪来、代价是什么**。

**核心权衡：** **执行速度（索取流动性）** vs **执行价格（提供流动性）**

---

## 1. 市价单 (Market Orders)

### 定义与特点

以当前市场 **最优可得价格** 立即成交 → **索取流动性 (Demand liquidity / Immediacy)**

### 成本与风险

| 项目 | 说明 |
|------|------|
| **买卖价差 (Bid-Ask Spread)** | 为即时性支付的显性成本 |
| **执行价格不确定性** | 成交价比预期更差 |
| **市场冲击 (Market impact)** | 大单推动价格；需价格让步才能找到对手 |

| HFT 视角 |
|----------|
| HFT **吃单 (take)** 时用 market / marketable limit |
| 冲击成本模型是 execution 与 **adverse selection** 核心 |
| 引擎侧：market order → 立即 match LOB best level |

---

## 2. 限价单 (Limit Orders)

### 定义与特点

以 **不劣于限价** 成交（买 ≤ 限价，卖 ≥ 限价）。未立即成交部分 **挂单 (Standing limit orders)** 进入 **限价指令簿** → **提供流动性 (Supply liquidity)**

### 定价策略（相对行情）

| 类型 | 位置 | 行为 |
|------|------|------|
| **Marketable 市价化限价** | 穿越 spread | 立即成交，类似市价单 |
| **At the market** | 最优买卖价 | 排队 |
| **In the market** | 价差内部 | 改善 BBO，形成新市场 |
| **Behind / Away** | 价差后方 | 被动等待 |

### 期权属性（核心洞见）

> **挂出的限价单 = 免费提供给对手方的交易期权 (Trading Options)**

- **卖出限价单** ≈ 赋予他人 **看涨期权**（允许对方在你价位买入）
- **买入限价单** ≈ 赋予他人 **看跌期权**（允许对方卖给你）

提交限价单者须以 **更优价格** 补偿让渡的期权价值。

| HFT 视角 |
|----------|
| 做市 = 持续 short options → 靠 spread + rebate 补偿 |
| 被 **picked off** = 期权被行权且方向错误 |
| Queue position = 期权被行权顺序 |

### 主要风险

**执行不确定性 (Execution uncertainty)**：价格远离限价 → **永不成交** → 机会成本（事后后悔）

---

## 3. 止损单 / 停止单 (Stop Orders)

### 定义

市价达到/越过 **停止价 (Stop price)** 时激活 → 转为 **市价单**。常见：**止损 (Stop loss)**

### vs 限价单

| | 止损单 | 限价单 |
|---|--------|--------|
| 卖出触发 | 价格 **变差** 时（跌破 stop） | 价格 **变好** 时（高于 limit） |

### 对市场的影响

下跌时集中触发卖 → **在流动性稀缺时索取流动性** → **加剧波动、增加动能 (Add momentum)**

| HFT 视角 |
|----------|
| Stop cascade = 流动性真空期的 **加速行情** |
| 需建模 **stop 簇** 与 flash crash 动态 |

---

## 4. 触及市价单 (Market-if-Touched, MIT)

### 定义

与止损相反：价格到达 **触及价** 时激活为市价单——**跌时买、涨时卖**

### 市场作用

交易方向 **逆** 市场运动 → **稳定市场 (Provide resiliency)**

| HFT 视角 |
|----------|
| 类似 contrarian liquidity；与 stop 形成 **动量 vs 均值回归** 两股力量 |

---

## 5. 价格变动敏感指令 (Tick-Sensitive Orders)

### 定义

执行条件取决于 **上一笔价格变动方向**：

- **Buy downtick**：仅下跌或平盘下跌时可买
- **Sell uptick**：仅上涨或平盘上涨时可卖

### 特点

- 动态限价，避免 **冲击市场**，倾向 **提供流动性**
- **十进制化 (Decimalization)** 后 tick 变小 → 吸引力 **大幅下降**

| HFT 视角 |
|----------|
| 历史 NYSE 规则；现代 tick size  debate (penny vs sub-penny) 影响 HFT 粒度 |

---

## 6. 不受约束市价单 (Market-Not-Held Orders)

客户赋予经纪人 **自由裁量权**——不必立即执行，由经纪人择时 **提供或索取流动性**。

- 典型：大机构委托 **场内专业经纪人** 的大额单

| HFT 视角 |
|----------|
| 与 HFT 自动化路径不同；human discretion vs microsecond execution |

---

## 7. 指令附加属性 (Order Instructions)

### 7.1 有效性与到期 (Validity / Expiration)

| 类型 | 说明 | HFT |
|------|------|-----|
| **Day** | 当日有效 | 默认 |
| **GTC** | 撤销前有效 | 长期挂单 |
| **IOC** | 立即成交否则撤销 | **HFT 常用**——不吃单不留痕 |
| **FOK** | 全部成交否则撤销 | 避免部分成交 |

> IOC/FOK 减少向市场 **赠送免费期权** 的时间

### 7.2 数量说明 (Quantity)

- **AON (All-or-none)**：全部成交或不成交——避免多次小单 **结算固定成本**
- **Minimum quantity**：最低成交量要求

### 7.3 显示说明 (Display / Hidden)

| 类型 | 目的 |
|------|------|
| **Hidden / Undisclosed** | 隐藏规模，防 **意图暴露** |
| **Iceberg 冰山** | 只展示一小部分数量 |

| HFT 视角 |
|----------|
| LOB 可见深度 ≠ 真实深度；解析 **hidden liquidity** 是 advanced LOB 技能 |
| 自身策略常用 iceberg 防被 front-run |

---

## 本章总结

```
市价单：付 spread → 换 immediacy     → 执行价格风险 + impact
限价单：赚 spread / 更好价 → 承担不成交 → 免费期权让渡
止损单：顺 momentum → 加剧波动
MIT：   逆 momentum → 稳定市场
附加属性：IOC/FOK/冰山 → 控制暴露与成交形态
```

> **HFT 读者 takeaway：** 第四章是 **订单 API 的业务语义**——写 Rust 发单模块时，每个 flag (limit/market/IOC/post-only/stop) 都对应本章的一类 economic trade-off。下一章 LOB 讲这些挂单 **如何在簿中排队与撮合**。

---

## 相关章节

- 上一章：[chapter-03-交易产业.md](./chapter-03-交易产业.md)
- 下一章：[chapter-05-市场结构.md](./chapter-05-市场结构.md)
