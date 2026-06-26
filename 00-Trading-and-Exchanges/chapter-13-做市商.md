# Ch 13 做市商 · Dealers

> **Trading and Exchanges** · Larry Harris · **精读** · Part IV · **流动性提供者**

本章详细探讨 **做市商 (Dealers)** 在市场中扮演的角色，以及如何通过 **设定报价、管理库存、应对风险**（尤其 **信息不对称**）获取利润。

> **HFT 核心章：** 电子 LOB 上 **挂限价单做市** = dealer 功能的 **去中介化**；与 [Ch 10 知情](./chapter-10-知情交易者与市场效率.md)、[Ch 14 价差](./chapter-14-买卖价差.md)、[Ch 12 虚张声势](./chapter-12-虚张声势者与市场操纵.md) 直接衔接；[00-practice-go-dex M3](./00-practice-go-dex/notes/milestone-03-价差与流动性/) 练手价差逻辑。

---

## 1. 核心定义与作用

### 1.1 提供流动性

**做市商**：**利润驱动型** 交易者，向希望 **立即成交** 的急躁交易者出售 **即时性 (Immediacy)** — 即 **流动性服务**。

| 角色 | 说明 |
|------|------|
| **客户** | 急切 **taker**（市价 / marketable limit） |
| **做市商** | **被动** 在 bid 买、在 ask 卖 |

### 1.2 盈利模式

**较低买价 (Bid) 买入 · 较高卖价 (Ask) 卖出** → 赚取 **买卖价差 (Bid-ask spread)**。

→ 价差分解详见 [Ch 14](./chapter-14-买卖价差.md)

### 1.3 被动交易者 (Passive Traders)

| 约束 | 含义 |
|------|------|
| **时机不由己** | 通常只在 **客户想交易时** 才成交 |
| **警惕** | 须对 **如何报价、与谁交易** 高度警惕 |

| HFT 视角 |
|----------|
| **Maker strategy** = 现代 dealer；**rebate** 是 spread 的 **费用表显式化** |
| 可 **主动撤单/改价**，比传统 floor dealer **控制力强**，但 **adverse selection** 逻辑不变 |

---

## 2. 吸引订单流与发现市场价格

### 2.1 吸引订单流 (Order Flow)

为盈利须吸引 **大量订单流**：

| 手段 | 说明 |
|------|------|
| **更有竞争力的价格** | 更窄 spread、更优 BBO |
| **更大展示规模** | 深度、可见 size |
| **客户服务** | 执行质量、研究（全服务 broker-dealer） |
| **购买订单流** | 向 broker **付钱** — [Ch 7 PFOF](./chapter-07-经纪人.md)、[Ch 25 内部化](./chapter-25-内部化优先撮合与交叉交易.md) |

### 2.2 寻找均衡：双向订单流

**终极目标**：发现 **市场出清价格 (Market-clearing price)** — 在该价位 **买卖双方意愿相互抵消** → **完美双向订单流 (Two-sided order flows)**。

| 理想状态 | 做市商 **低库存、高 turnover** — 反复 **低买高卖** 赚 spread |
|----------|----------------------------------------------------------------|

| HFT 视角 |
|----------|
| **Balanced flow** 日 inner spread 最稳；**单向 toxic flow** → inventory 堆积 |
| **Mid-price targeting**、**skew to attract opposite side** |

---

## 3. 库存管理与两大库存风险

**库存 (Inventories)** = 做市商 **手头头寸**；通常有 **目标库存 (Target inventories)**。

库存 **偏离目标** → 两类风险：

### 3.1 可分散的库存风险 (Diversifiable Inventory Risk)

| 来源 | **不可预测的随机价格波动** |
|------|---------------------------|
| **性质** | 相对温和 — 涨跌 **概率大致相等** |
| **分散** | 同时交易 **多种工具** 分散 |

| HFT 视角 |
|----------|
| **Multi-symbol MM**、**beta hedge** 降低 idiosyncratic inventory risk |
| **Vol scaling** — 高 vol 日缩小 per-name size |

### 3.2 逆向选择风险 (Adverse Selection Risk) — 最致命

**与知情交易者 (Informed traders) 交易的风险**（[Ch 10](./chapter-10-知情交易者与市场效率.md)）。

| 模式 | 知情者在 **将涨时买、将跌时卖** |
|------|--------------------------------|
| **结果** | 与做市商成交后，价格常向 **不利于做市商** 方向走 → **亏损** |

| HFT 视角 |
|----------|
| **Maker PnL** 核心减法项；**VPIN / toxicity** 度量 |
| **Quote fade**  after large informed hit |

### 3.3 库存控制：调整报价

| 库存状态 | 报价调整 | 意图 |
|----------|----------|------|
| **太少，想买入** | **同时提高** bid **和** ask | 鼓励别人 **卖给你**；阻碍别人 **从你买走** |
| **太多，想清仓** | **同时降低** bid **和** ask | 鼓励别人 **买入**；减少继续 **卖给你** |

> **关键：** 双边 **同向移动**（shift whole quote），而非只动一侧 — 在 **维持 spread 宽度** 的同时 **偏移 mid** 调节 inventory。

| HFT 视角 |
|----------|
| **Inventory skew algorithm**：`mid' = mid + γ × (inventory − target)` |
| Go DEX / Rust 引擎若只做 **matching** 无 **自营 skew** — 用户各自为 maker 承担此逻辑 |

---

## 4. 应对知情交易者的策略

做市商 **难直接识别** 知情者 → 须从 **订单流推断** 基本面变化，并采取防御：

### 4.1 迅速调整报价

若怀疑刚 **买入了知情者的抛售**：

| 动作 | 目的 |
|------|------|
| **立即降低双边报价** | 阻止其他知情者继续 **卖给你** |
| | **降价吸引不知情买家** — 在 **真跌前** 清空库存 |

| HFT 视角 |
|----------|
| **Microsecond quote update** after fill；**cancel-replace storm** |
| **Auto-hedge** 到 futures / ETF |

### 4.2 拉宽价差：逆向选择价差成分

将 **遇到知情者的概率** 与 **预期损失** **预埋** 进报价 → ask **提高**、bid **降低**。

**额外加宽部分** = **逆向选择价差成分 (Adverse selection spread component)**。

→ [Ch 14](./chapter-14-买卖价差.md) 完整分解（inventory + adverse selection + monopoly 等）

| HFT 视角 |
|----------|
| **Widen spread when toxicity ↑**；**vol regime** 联动 |
| **Maker rebate** 须 **覆盖** expected adverse selection |

### 4.3 提防大单与匿名交易

| 信念 | 行为 |
|------|------|
| **大单更可能知情** | 报 **极宽价差** 或 **只显示小 size** |
| **匿名 = 隐藏身份** | **拒绝或回避** — 防 informed **stealth** |

| HFT 视角 |
|----------|
| **Size skew**、**max fill size**、**ISO / directed** 规则 |
| **Dark pool** 与 **lit market** 的 **adverse selection 不对称** |

---

## 5. 做市商与其他交易者的博弈

### 5.1 与价值交易者 (Value Traders)

| 触发 | 做市商为 **极端库存** **大幅改价** → 价格 **偏离基本面** |
|------|----------------------------------------------------------|
| **价值交易者入场** | 识别 mispricing → **反向交易** |
| **角色** | 充当做市商的 **终极流动性提供者** — 帮其 **卸载 (Lay off)** 巨大风险头寸 |

→ [Ch 16](./chapter-16-价值交易者.md)

| HFT 视角 |
|----------|
| **External capital** 接盘 inventory — 做市商不必 **无限扛单** |
| **Hedge fund value** 与 **HFT MM** 在 **mispricing** 上 **共生** |

### 5.2 与虚张声势者 (Bluffers)

| Bluffer 利用 | 做市商 **根据单向 flow 调价** 的习惯 |
|--------------|--------------------------------------|
| **手段** | **故意制造失真交易量** → 诱导 **错误改价** |
| **防守** | [Ch 12](./chapter-12-虚张声势者与市场操纵.md) — **对称 impact 纪律**；同等数量买卖须产生 **对称价格冲击** 才调价 |

| HFT 视角 |
|----------|
| **Don't chase one-sided wash volume** |
| **Surveillance + impact linearity** 防 **被调教 (grooming)** |

---

## 6. 生存法则（扑克类比）

```
销售即时性 → 赚 spread
      │
      ├── 根据订单流猜「底牌」（基本面信息）
      ├── 灵活调价 → 库存平衡
      └── 尽一切可能避免成为知情者的「待宰羔羊」
```

| 收入 | 成本 / 风险 |
|------|-------------|
| **Spread capture** | **Diversifiable inventory risk**（温和） |
| | **Adverse selection**（致命） |
| | **Bluffer 诱导错误 skew**（纪律可防） |

---

## 7. 本章总结

| 要点 | 含义 |
|------|------|
| **角色** | 卖 **immediacy**；被动成交 |
| **盈利** | Bid-ask spread；须 **吸引 order flow** |
| **理想** | 出清价附近 **双向 flow** |
| **库存风险** | 可分散（温和）vs **逆向选择**（致命） |
| **调价** | 库存低/高 → **双边同向** 移价 |
| **防 informed** | 快撤、**加宽 adverse selection 成分**、警惕大单/匿名 |
| **盟友** | **价值交易者** 帮 unload；**敌人** bluffer 利用 skew 习惯 |

> **HFT 读者 takeaway：** 做 maker = 本章主角。读 `orderbook.go` 时分清：**matching 规则**（Ch 6）vs **做市商 skew 策略**（本章）— 前者是交易所 spec，后者是你的 **PnL 引擎**。下一章 [Ch 14](./chapter-14-买卖价差.md) 把 spread **拆成可估分量**。

---

## 相关章节

- 上一章：[chapter-12-虚张声势者与市场操纵.md](./chapter-12-虚张声势者与市场操纵.md)
- 下一章：[chapter-14-买卖价差.md](./chapter-14-买卖价差.md)
- 知情交易：[chapter-10-知情交易者与市场效率.md](./chapter-10-知情交易者与市场效率.md)
- 订单流与 PFOF：[chapter-07-经纪人.md](./chapter-07-经纪人.md) · [chapter-25-内部化优先撮合与交叉交易.md](./chapter-25-内部化优先撮合与交叉交易.md)
- 练手：[00-practice-go-dex M3 价差与流动性](./00-practice-go-dex/notes/milestone-03-价差与流动性/)
