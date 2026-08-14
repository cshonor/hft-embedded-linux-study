## 2. 交易执行系统 (Execution Systems)

市场结构中 **最重要的特征**——买方与卖方如何匹配。

### 2.1 报价驱动 (Quote-driven / 做市商市场)

- **做市商参与每一笔交易**，提供 **全部流动性**
- 公众 **不能直接互成交**，须与做市商（或其 broker）谈判

| 特征 | HFT |
|------|-----|
| Dealer 控 spread | 传统 Nasdaq 结构；现代仍见 **internalization** |
| 无 peer-to-peer | HFT 若做市 = 扮演 dealer 角色 |

### 2.2 指令驱动 (Order-driven)

- 买卖双方 **直接互成交**，无需做市商介入
- 靠 **交易规则** 匹配：**指令优先规则** + **交易定价规则**
- 流动性来自公众 **限价单**

| 特征 | HFT |
|------|-----|
| 拍卖 / 电子撮合 | **现代 equity HFT 核心**：LOB + price-time priority |
| 规则即代码 | matching engine 实现 = 本章 abstract → 代码 |

### 2.3 经纪人市场 (Brokered markets)

- 经纪人 **主动搜索** 对手方
- 用于 **大宗**、房地产、**极低流动性** 工具

| HFT 视角 |
|----------|
| 与 HFT 热路径分离；block negotiation upstairs |

### 2.4 混合市场

现实市场常 **组合** 上述类型（如 exchange LOB + designated market maker + broker upstairs）。

---
