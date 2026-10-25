# 第6章 策略回测框架实现

> 回测 = 用历史或合成行情，走 **和实盘同一套** Book / Strategy / Risk。差别只在「成交谁给」。

← [第5章](./chapter-05-量化策略模型开发.md) · 下一章：[第7章 引擎](./chapter-07-实盘交易引擎开发.md)

---

## 本节讲什么

回测框架只有一条正确性标准：**回测和实盘链同一份代码**。本章把 demo 的回放管道（`Replay::events()` → `Engine::handle` → 报表）逐段拆开，讲清三个问题：合成行情是怎么「编」出来的、`run_strategy` 标志为什么是一拍的边界、以及三种最常见的回测作弊在这个 demo 里分别被什么机制挡住。

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

### demo 的回放管道

[`demo/src/replay.rs`](./demo/src/replay.rs) 不接任何真实交易所，它在内存里「编」一个会走的市场。每个 tick 产出 4~5 个 `Event`，顺序是固定的：

```
tick i:
  fair 随机游走一步（±1 tick；每 80 拍叠加一次 ±12 tick 的跳价）
  → cancel(other_bid)            # 撤掉「别人」上一拍的买盘
  → cancel(other_ask)            # 撤掉「别人」上一拍的卖盘
  → order(new_bid, run=false)    # 按新 fair 重挂别人买盘（fair - 4 tick）
  → order(new_ask, run=!hit)     # 重挂别人卖盘（fair + 4 tick）
  → [35% 概率] order(market_hit, run=true)   # 市价冲击单，先撞我们的窄报价
收尾: shutdown
```

对应代码坐标：`replay.rs:113-171` 是主循环，`run_strategy` 标志在 `:159-161` 设置（没有冲击单时 ask 入簿即触发策略；有冲击单时由冲击单触发），`shutdown` 在 `:173` 追加。

注意「别人」的订单 id 从 1 起（`replay.rs:109` 的 `id_seq`），而我们的报价 id 从 `1_000_000_000` 起（`engine.rs:92`）。两个命名空间错开不是洁癖：撤单按 id 查索引，撞号意味着你可能撤掉别人的单，簿会**静默**少一档，这种 bug 极难查。

### run_strategy 是一拍的边界

一个 tick 里别人可能先撤、再挂、再来一笔市价冲击。策略必须等**这一串市场动作做完**再决策，否则就挂在过时的 BBO 上。这就是 `Event.run_strategy` 字段（`types.rs:83`）存在的全部理由。

引擎侧的实现在 `engine.rs:117-119`：

```rust
if !e.run_strategy {
    return; // 这一笔只是市场更新，还没到我们决策的点
}
```

`cancel` 事件的 `run_strategy` 恒为 `false`（`types.rs:96-103`），所以撤单永远只更新簿、不触发报价。这个语义和 P10 `replay.hpp` 完全一致：「ask 入簿后或冲击后才 `run_strategy`」。

### 随机数：能复现比对是第一需求

`Replay` 不用 `rand` crate，自带一个 17 行的 xorshift64（`replay.rs:48-88`）：

```rust
fn next(&mut self) -> u64 {
    let mut x = self.0;
    x ^= x << 13;
    x ^= x >> 7;
    x ^= x << 17;
    self.0 = x;
    x
}
```

选它的理由有三条，条条是工程理由而不是密码学理由：

| 需求 | xorshift64 怎么满足 |
|------|--------------------|
| 零依赖 | 17 行，不拉 `rand` 进依赖树 |
| 可复现 | 同一个 `--seed` 产出**逐字节相同**的事件流 |
| 够均匀 | 编市场用，不用于密钥；`chance(p)` 用 `next() % 10_000 < p*10000` 做概率判定（`replay.rs:78-87`） |

种子固定（`ReplayConfig.seed`，默认 1）是回测的硬要求：两次回放对不了账时，你必须能分清是「策略改了」还是「行情变了」。种子不固定，这个区分就没了。

### ReplayConfig 参数表

| 字段 | 默认 | 含义 | 调它看什么 |
|------|------|------|-----------|
| `ticks` | 20_000 | 回放长度 | 统计显著性 |
| `start_fair` | 10_000 | 起始公允价（= 100.00 元） | — |
| `other_half` | 4 | 「别人」挂单离 fair 的半价宽 | 我们 ±2 的报价挂在别人 ±4 里面，才赚得到价差 |
| `other_size` | 40 | 别人每档的量 | 冲击单吃不吃得穿 |
| `jump_every` | 80 | 每 N 拍跳一次价；`0` = 关掉 | **A/B 对照的核心开关** |
| `jump_ticks` | 12 | 跳价幅度 | 逆向选择的强度 |
| `hit_prob` | 0.35 | 每拍出市价冲击的概率 | 成交频率 |
| `hit_max` | 12 | 冲击单最大量 | 单笔库存冲击 |
| `seed` | 1 | 随机种子 | 复现 |

`--jump 0` 是最重要的对照实验：跳价 + 冲击 = 逆向选择——你最容易成交的时候，往往是价格已经对你不利。关掉跳价后 PnL 的变化，就是策略为逆向选择付的学费。这条结论只信自己跑出来的数字，不信表格里的默认值。

---

## 三种作弊（必须防）

回测骗你的方式只有三种，各有各的防法：

| 作弊 | 样子 | 防法 |
|------|------|------|
| 未来函数 | 用了这根未收盘 K 的 close | 只看已关闭 bar |
| 上帝成交 | 市价永远打在当时 mid，无滑点 | 至少按买卖一档成交 |
| 偷看全市场 | 回测用了实盘当时看不到的字段 | 输入必须是当时可获得的 feed |

demo 怎么挡住「上帝成交」：成交价 = **maker 挂单价**（价格改善归 taker）。机制在 `book.rs:264-273`：`match_level` 里 `trades.push(Trade { price: px, .. })` 的 `px` 是**被吃那一档的价格**，不是 mid、也不是 taker 的限价。这和 P10 / 真实 LOB 一致。不要改成「成交在 mid」——那等于假设你每次都能白捡半个价差，回测会系统性地高估收益。

`cargo test` 里的 `best_price` 测试（`lib.rs:36-46`）锁死的就是这条：簿上卖 103 与 104，买 105 进来必须先成交在 **103**，且成交价是 103 不是 105。

滑点、手续费可以先做成常数，但**不能是 0 还对外声称夏普**。对照 [14 §10.2](../14-hft-engineering/chapter-10-risk-compliance-slippage/10.2-滑点度量.md)。本 demo 手续费 = 0，所以报表里的 PnL 只能用来对比 `--jump`，不能当真实收益。

---

## 和实盘共用代码

```
engine/     ← Book + Strategy + Risk   回测、仿真、实盘都链这份
backtest/   ← 只负责读文件、推进时间、出报表
live/       ← 只负责 socket / 会话
```

「回测一份 Python、实盘一份 C++」是事故源。Rust 的目标之一就是这两边链同一个 `engine` crate。demo 已经是这个形态：`lib.rs` 是引擎语义（`book` / `engine` / `replay` / `risk` / `strategy` / `types` 六个 mod），`bin/hft_demo.rs` 只负责读 CLI、打报表。

验收方式是 `replay_pipeline_runs` 测试（`lib.rs:267-275`）：300 tick 的完整回放跑完，`stats.ticks == 300` 且 `last_mid != INVALID_PRICE`——证明从事件生成到报表的整条管道是通的，而不是只测了某个孤立函数。

---

## HFT/嵌入式关联

- **回测/实盘同一份二进制**在嵌入式里是同构问题：固件的「仿真模式」和「上板模式」必须链同一份控制律，差别只在传感器输入从哪来。做法一样——输入抽象成事件流，核心逻辑不认输入来源。
- **固定种子可复现**对应嵌入式里的「记录-回放」调试：现场抓的输入序列存下来，回实验室逐拍重放定位。没有确定性重放，偶发 bug 永远修不掉。
- 回测的延迟数字（demo 报表里的 p50/p99）只在**同一台机器、同一份事件流**之间可比；跨机器比绝对值没有意义，和嵌入式 benchmark 要绑平台报数字是同一纪律。

---

## 代码自测

<details>
<summary>1. 为什么别人的订单 id 从 1 起、我们的从 1_000_000_000 起？撞号会发生什么？</summary>

撤单按 id 查 `index: HashMap<u64, BookLoc>`（`book.rs:131-133`）。两个命名空间若重叠，引擎撤自己旧报价时可能撤掉别人的挂单——簿静默少一档，BBO 变化找不出原因。错开 id 区间是最便宜的防错。
</details>

<details>
<summary>2. 一个 tick 内有冲击单时，`run_strategy` 挂在哪个事件上？为什么这么安排？</summary>

挂在市价冲击单上（`replay.rs:169` 的 `Event::order(hit, true)`），而 ask 入簿事件带 `false`（`:161` 的 `!will_hit`）。因为冲击可能改变 BBO——先挂 ask 就决策等于用过时的簿报价。
</details>

<details>
<summary>3. 成交价为什么必须用 maker 挂单价而不是 mid？改成 mid 会怎样？</summary>

真实撮合里价格改善归 taker：买单吃 103 的卖单就成交在 103。改成 mid 等于每笔白送半个价差，回测系统性高估收益——这是「上帝成交」作弊的一种。`lib.rs:36-46` 的 `best_price` 测试锁死这条语义。
</details>

<details>
<summary>4. demo 的 PnL 数字（手续费 = 0）能当策略收益对外说吗？</summary>

不能。它只对「同一事件流上开/关跳价」这类 A/B 对照有效。没有手续费和滑点模型的 PnL，对外声称夏普就是作弊三表里的第二条变体。
</details>

---

## 卡住翻哪篇

| 卡住了… | 翻这里 |
|---------|--------|
| P10 回放设计 | [design.md §7](../projects/P10-hft-prototype/docs/design.md) |
| 撮合三种场景 | [14 §3.2](../14-hft-engineering/chapter-03-orderbook-depth-market-data/3.2-撮合引擎三种场景.md) |
| 滑点度量 | [14 §10.2](../14-hft-engineering/chapter-10-risk-compliance-slippage/10.2-滑点度量.md) |
| 本模块回放 | [`demo/src/replay.rs`](./demo/src/replay.rs) · [`engine.rs`](./demo/src/engine.rs) |
