# 第3章 交易所动态与订单簿

> **原书第 3 章 · Understanding the Trading Exchange Dynamics**  
> **撮合引擎 · FIFO/Pro-rata · 共址 · 本地 Book 启示**

← [chapter-02 关键组件](./chapter-02-交易所架构与撮合原理.md) · 总览：[chapter-01](./chapter-01-高频交易基础与生态.md)

---

## 本章定位

第二章构建 **我方交易系统**；第三章转向 **市场另一端——交易所**。

在 μs/ns 竞争中，**微观结构（Microstructure）** 与 **撮合规则** 是策略能否盈利的 **绝对前提** — 决定你是 **抢队列** 还是 **堆量 Pro-rata**。

→ 理论深化：[00-Trading-and-Exchanges](../00-Trading-and-Exchanges/)

---

## 1. 交易所核心功能与架构

现代交易所已由 **Open Outcry（大厅喊价）** 转为 **全自动计算机撮合**。

| 组件 | 职责 |
|------|------|
| **Listings（上市）** | IPO、挂牌 |
| **Matching Engine（撮合引擎）** | **心脏** — ns 级发布订单簿、匹配买卖 |
| **Post-trade（盘后）** | 付款、结算、对账；**保证金** 防违约 |
| **Market Data（行情）** | 海量分发 → 催生 **Co-location（共址）** — 物理最近接 feed |
| **Regulation（监管）** | 操纵监控、透明度 |

```
Your HFT ──order──► Router ──► per-symbol/per-price FIFO queue ──► Matching Engine
                         ▲
              Market Data feed (co-located)
```

---

## 2. 订单进入撮合的路径

1. 交易系统 **Gateway OUT** 发单至交易所  
2. **Router** 按 **symbol（AAPL、MSFT…）** 路由  
3. 按 **价格档位** 进入 **FIFO 队列**（在 FIFO 规则交易所）  
4. **Matching Engine** 消费新单 + 当前 LOB → **Trade** 或 **Resting order**

**HFT 含义：** 你看到的 **queue position** 由 **到达时间 + 是否 amend 丢优先级** 决定。

---

## 3. 撮合引擎三种基础场景

**输入：** 新订单 + 当前订单簿 → **输出：** 成交记录和/或 **挂单（Resting）**。

### 最佳价格（Best Price）

- 引擎 **始终为新单找最优价**  
- 例：买 @$100，簿上有卖 $99 与 $100 → **先以 $99 成交**（价格改善）

### 部分成交（Partial Fill）

- 买 4 股，最优卖单仅 1 股 → **先成交 1**  
- 剩余 3 股：**继续吃下一档** 或 **挂入簿中**

### 无匹配（No Match）

- 买 @$98，最低卖 $99 → **不成交**，整单 **Rest** 在簿中提供流动性

**本地 Book Builder** 必须用 **相同语义** 重建 feed，策略才能 **模拟 queue / fill**。

---

## 4. 同价匹配算法（核心）

多笔订单 **同价** 时，「谁先成交」**决定 HFT 生死**：

| 算法 | 规则 | HFT 策略 |
|------|------|----------|
| **FIFO（Time-Price）** | **时间戳最早** 优先；**改价**（部分所 **改量也**）→ **丢队尾** | **抢 1 ns** 排前；少 **amend** |
| **Pure Pro-rata** | 按 **挂单量比例** 分新单；**不看** 先后 | **加大 size** 换份额；速度次要 |
| **Pro-rata + Top-order** | **队首老单 100% 满额** → 余量 **按比例** | **速度 + 量** 混合博弈 |

### FIFO 的致命细节

> 修改价格（甚至数量）→ **失去原队列优先级** → 重新排到 **队尾**。

**推论：**

- 优化 **T2T / 共址** 在 FIFO 市场 **ROI 极高**
- **Cancel-Replace 策略** 需计 **re-queue 成本**

### 算法选择 → 系统设计

| 若交易所是… | 你应该… |
|-------------|---------|
| **FIFO** | 疯狂优化 **网络延迟**、「抢排头」 |
| **Pro-rata** | **加大挂单尺寸**、算 **份额公式** |
| **Top-order Pro-rata** | **既要快又要大** — 双轨优化 |

→ [chapter-13 策略](./chapter-13-高频做市与套利策略.md) · [chapter-04 共址/硬件](./chapter-04-硬件选型与服务器配置.md)

---

## 5. 共址与市场数据

| 概念 | 说明 |
|------|------|
| **Co-location** | 机柜与 **撮合/行情源** 同机房 — **减传播延迟** |
| **Market Data** | 增量 feed 驱动 **本地 LOB** — 与引擎 **逻辑一致** |

**1 ns** 在 FIFO 中即可形成 **执行优势** — 与 [chapter-10 T2T](./chapter-10-延迟测量与基准压测.md) 测量目标一致。

---

## 6. 本地 Book Builder 与行情解析（我方系统）

理解交易所动态后，**Gateway IN + Book Builder** 必须：

| 工程要点 | 说明 |
|----------|------|
| **O(1) 更新** | add/modify/delete/trade |
| **Vector / 预分配** | 高 **cache hit** — 见 [chapter-02 §3](./chapter-02-交易所架构与撮合原理.md#3-订单簿构建器-book-builder) |
| **Feed 解析** | Wire → struct；**gap → snapshot recovery** |
| **Normalize** | 多交易所 **统一内部 Event** |

→ 实现：[chapter-08 Book Builder](./chapter-08-超低延迟核心引擎开发.md#2-book-builder) · [chapter-06 协议](./chapter-06-低延迟网络与协议优化.md)

---

## 本章小结

| 交易所侧 | 对你系统的含义 |
|----------|----------------|
| **Matching Engine** | 理解 **Fill / Rest** 三种场景 |
| **FIFO vs Pro-rata** | 决定 **延迟 vs 尺寸** 策略 |
| **Amend 丢优先** | 少改单 · 优化 **Gateway OUT** |
| **Co-location** | **Gateway IN/OUT** 物理布局 |

**下一步：** [chapter-04 硬件到 OS](./chapter-04-硬件选型与服务器配置.md) · [chapter-05 OS 调优](./chapter-05-操作系统内核极致调优.md)
