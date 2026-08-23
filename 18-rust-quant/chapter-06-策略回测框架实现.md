# 第6章 策略回测框架实现

> 回测 = 用历史或合成行情，走 **和实盘同一套** Book / Strategy / Risk。差别只在「成交谁给」。

← [第5章](./chapter-05-量化策略模型开发.md) · 下一章：[第7章 引擎](./chapter-07-实盘交易引擎开发.md)

---

## 正确的回测

```
事件流（文件 / 生成器）
    → 与实盘相同的 on_event
    → 模拟撮合或「假定挂在 BBO 能立刻成交」
    → 记录成交、库存、标记 PnL
```

P10 把「交易所」和「我们」放在同一本簿上，回放线程扮演别人——这已经是最小回测。

本模块现在也接上了：`Replay::events()` → `Engine::handle` → 报表。`cargo test` 测规则；`cargo run --release` 才是这段回放。不要再写第二套 Python 撮合「验证」Rust 结果——对不齐时你分不清是谁错。

---

## 三种作弊（必须防）

| 作弊 | 样子 | 防法 |
|------|------|------|
| 未来函数 | 用了这根未收盘 K 的 close | 只看已关闭 bar |
| 上帝成交 | 市价永远打在当时 mid，无滑点 | 至少按买卖一档成交 |
| 偷看全市场 | 回测用了实盘当时看不到的字段 | 输入必须是当时可获得的 feed |

demo 的成交价 = **maker 挂单价**（价格改善归 taker），和 P10 / 真实 LOB 一致。不要改成「成交在 mid」。

滑点、手续费可以先做成常数，但 **不能是 0 还对外声称夏普**。对照 [14 §10.2](../14-hft-engineering/chapter-10-risk-compliance-slippage/10.2-滑点度量.md)。本 demo 手续费 = 0，所以报表里的 PnL 只能用来对比 `--jump`，不能当真实收益。

---

## 和实盘共用代码

```
engine/     ← Book + Strategy + Risk   回测、仿真、实盘都链这份
backtest/   ← 只负责读文件、推进时间、出报表
live/       ← 只负责 socket / 会话
```

「回测一份 Python、实盘一份 C++」是事故源。Rust 的目标之一就是这两边链同一个 `engine` crate。demo 已经是：`lib` = 引擎语义，`bin` = 只负责读 CLI、打报表。

随机种子要固定（`--seed`），否则两次回放对不了账，你会以为策略不稳。

---

## 卡住翻哪篇

| 卡住了… | 翻这里 |
|---------|--------|
| P10 回放设计 | [design.md §7](../projects/P10-hft-prototype/docs/design.md) |
| 撮合三种场景 | [14 §3.2](../14-hft-engineering/chapter-03-orderbook-depth-market-data/3.2-撮合引擎三种场景.md) |
| 本模块回放 | [`demo/src/replay.rs`](./demo/src/replay.rs) · [`engine.rs`](./demo/src/engine.rs) |
