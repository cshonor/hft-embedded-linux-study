# 第5章 量化策略模型开发

> Strategy = Signal（何时）+ Execution（如何下）。热路径上的策略是函数，不是框架。

← [第4章](./chapter-04-时间序列与K线处理.md) · 下一章：[第6章 回测](./chapter-06-策略回测框架实现.md)

---

## 接口（概念）

```
on_book(bbo, inventory) → 一小组意图（新单 / 撤单）
on_fill(trade)          → 改库存和现金
```

P10 的 `MarketMaker::quote` / demo [`strategy.rs`](./demo/src/strategy.rs) 就是这个。不要在 `on_book` 里读文件、分配大 `String`、调 HTTP。

做市核心等式（[14 §12.1](../14-hft-engineering/chapter-12-market-making-arbitrage/12.1-做市核心等式与逆向选择.md)）：

```
PnL = 成交量 × 价差 − 逆向选择 − 库存风险
```

库存多 → 报价整体下移，让市场帮你卖掉。P10 / demo 都是：

```
skew = (inventory / skew_unit) * skew_per_pos
bid  = mid - half - skew
ask  = mid + half - skew
```

默认 `half=2`、市场别人 `half=4`，所以我们挂在里面；别人来吃才赚价差。跳价打到旧报价时，你赚到的是「已经过时的价」——这就是逆向选择。`--jump 0` 对比的就是这一项。

注意：每 tick 撤旧挂新，在 FIFO 市场会丢掉队列位置。教学简化，实盘要算「丢队首值不值」（第 8 章）。

---

## 研究策略 vs 热路径策略

| | 研究 | 热路径 |
|--|------|--------|
| 输入 | 历史 K 线、因子矩阵 | BBO + 库存 + 少量标志 |
| 输出 | 「方向 / 权重」报告 | 具体限价、数量 |
| Rust | 可以 `Vec`、可以慢 | 预分配、分支可预测 |

因子模型、机器学习放 **冷进程**，用队列把「偏斜参数 / 开关」喂给热路径。热路径只读一份原子参数快照。

---

## Rust 落点

- 用泛型 `fn run<S: Strategy>(…)` 而不是 `Box<dyn Strategy>` 打在每个 tick 上。  
- 意图用小型 struct / enum，不要 `HashMap<String, Order>`。  
- 策略 **不算** 最终能否下单——那是 Ch9 风控的独立代码路径。  
- `on_fill` 必须能区分 taker / maker：我们是挂单被吃，还是我们去吃别人。demo `MarketMaker::on_fill` 按 `owner` 判断。

`vol_ema` 用了 `f64`：它只加宽价差，**绝不拿去撮合**。撮合路径仍然全是 `i64`。

---

## 卡住翻哪篇

| 卡住了… | 翻这里 |
|---------|--------|
| Signal 与 OMS | [14 §2.3](../14-hft-engineering/chapter-02-exchange-architecture-matching/2.3-策略引擎与OMS.md) |
| P10 做市实现 | [P10 `strategy.hpp`](../projects/P10-hft-prototype/part-a-demo/src/strategy.hpp) |
| 本模块代码 | [`demo/`](./demo/) `MarketMaker` |
