# 第8章 订单管理与路由系统

> OMS 记住每张单的一生；路由决定发去哪个所。风控在出门前，不在 RTT 之后。

← [第7章](./chapter-07-实盘交易引擎开发.md) · 下一章：[第9章 风控](./chapter-09-风险控制与仓位管理.md)

---

## 本节讲什么

demo 没有独立的 OMS 对象，但 OMS 的核心问题它一个都躲不掉：在途单怎么记、撤单怎么定位、自成交怎么防。本章把这三个问题在 demo 里的最小实现（`live_bid_id` / `live_ask_id`、`index: HashMap<u64, BookLoc>`、STP 跳过）拆开，再讲生产级 OMS 要补什么。路由部分只讲清「单市场 demo 为什么不需要路由」这一退化的理由。

---

## 生命周期

```
Created → Sent → Ack → PartialFill → Done
                   ↘ Reject / Cancelled
```

策略只产生「意图」。OMS 才持有 `order_id`、剩余量、是否在途。重复点撤、乱序成交回报，都在这里对账。

对照 [14 §2.3](../14-hft-engineering/chapter-02-exchange-architecture-matching/2.3-策略引擎与OMS.md)：违规单 **本地拒**，不要占用到交易所的往返。demo 还没有独立 OMS 对象：`live_bid_id` / `live_ask_id`（`strategy.rs:40-41`）就是最小「在途表」——每边最多记一张在途单的 id。生产要把状态机写全，否则会重复挂单或撤已经没了的单。

### demo 的在途表：两个 u64 能走多远

引擎维护在途单的全部代码只有两处：

```rust
// engine.rs:126-133 —— 拍开始，撤旧
if self.mm.live_bid_id != 0 {
    let _ = self.book.cancel(self.mm.live_bid_id);
    self.mm.live_bid_id = 0;
}
// engine.rs:146-149 —— 新单成交/入簿后，记新 id
match side {
    Side::Buy => self.mm.live_bid_id = id,
    Side::Sell => self.mm.live_ask_id = id,
}
```

这个最小形态能成立，靠的是两个简化前提：

| 前提 | 生产里为什么不成立 |
|------|------------------|
| 每边最多一张在途单 | 生产有多档做市、冰山单、多策略并发 |
| 撤单同步完成（`book.cancel` 本地返回） | 生产的 cancel 是一次网络往返，期间单子可能已成交 |
| 没有部分成交的状态 | 生产的 PartialFill 要更新剩余量，不能简单地「id 清零」 |

所以 demo 的两个 `u64` 是**状态机的退化形态**：状态只有「无在途 / 有在途 id」两态。补上「撤单在途」「部分成交」「拒单」三个状态，就是生产 OMS 的骨架。

### 撤单路径：cancel() 内部

`OrderBook::cancel`（`book.rs:131-153`）分四步：

```
① index.remove(&id)         → 拿不到 BookLoc 就返回 false（单不在簿上）
② 按 loc.side 选 bids/asks  → 按 loc.price 取 PriceLevel
③ 在 VecDeque 里按 id 线性找 → remove(pos)，lvl.total -= qty
④ 价位空了 → map.remove(&loc.price)
```

复杂度如实说：① 是 O(1)，③ 是 O(该价位排队单数)。「撤单 O(价位队列扫描)，正确性优先」——`VecDeque` 保 FIFO 序，按 id 扫描是正确性最直白的写法。生产要 O(1) 撤单就得在 `BookLoc` 里再存迭代器位置或换侵入式链表，那是 [14 §7.2](../14-hft-engineering/chapter-07-lockless-data-structures-memory-layout/7.2-无锁FIFO队列.md) 之后的事。

`index` 的存在理由写在 `book.rs:31` 的注释里：「字段不对外可变：撮合必须走 `submit` / `cancel`，否则 FIFO 索引会坏」。私有字段 + 公开方法，是 Rust 里「不变量由类型守卫」的标准做法——外部代码没有路径绕过 `index` 直接改 `bids`/`asks`。

---

## Cancel-Replace

FIFO 市场里改价 = 撤掉重挂 = **去队尾**（[14 §3.3](../14-hft-engineering/chapter-03-orderbook-depth-market-data/3.3-同价匹配算法.md)）。  
P10 / demo 每 tick 撤旧挂新，是教学简化（`strategy.rs:6` 注释：「每 tick 撤旧挂新，在 FIFO 市场会丢掉队列位置。这是教学简化。」），实盘要算「丢队首值不值」。价没变就不要撤。

Rust 里用 `HashMap<u64, LiveOrder>` 管在途单；热路径避免按 `String` 当 key。

### 自成交保护（STP）：两道闸门

策略同一拍挂买又挂卖，若买价 ≥ 卖价就会自己跟自己成交——账面库存不变但白付两次手续费，监管上还涉嫌虚假成交。demo 有两道闸门：

**闸门一：策略侧不产交叉价**（`strategy.rs:135-137`）：

```rust
if ask_px <= bid_px {
    ask_px = bid_px + 1; // 买价不能超过卖价，否则自己跟自己成交
}
```

**闸门二：撮合侧跳过同 owner**（`book.rs:259-263`）：

```rust
while incoming.qty > 0 && i < lvl.orders.len() {
    if incoming.owner != OWNER_MARKET && lvl.orders[i].owner == incoming.owner {
        i += 1; // 自成交保护：跳过自己的挂单，继续找别人
        continue;
    }
    ...
}
```

注意闸门二的豁免条件 `incoming.owner != OWNER_MARKET`：市场侧 `owner=0` 表示「许多人」，彼此可以成交（`book.rs:231-234` 的 `qty_from_others` 同样跳过这个判断）。`stp_same_owner_must_not_match` 测试（`lib.rs:150-173`）锁死这条：同 owner 的买卖同价挂上去，零成交、簿上两边同时存在。

FOK 的可用量检查也走 STP 过滤（`book.rs:155-179` 的 `available_to_fill` 调 `qty_from_others`）——否则 FOK 会把「自己的单」算进可成交量，判出假阳性。

---

## 订单类型

demo `cargo test` 全覆盖（`lib.rs` 的 `best_price` / `partial_fill` / `no_match` / `market_walks_two_levels` / `ioc_remainder_does_not_rest` / `fok_rejects_when_size_insufficient`）：

| 类型 | 行为 | demo 实现坐标 |
|------|------|--------------|
| Limit | 能成则成，剩余排队 | `book.rs:125-127`（只有 Limit 走 `rest_order`） |
| Market | 能吃多少吃多少，不挂 | `book.rs:104-111`（买单价改写成 `MAX_PRICE`，卖单改 0，复用同一套 `crosses()`） |
| IOC | 立刻成，剩余扔掉 | 撮合完不 rest，剩余量自然丢弃 |
| FOK | 不够量则整单不做 | `book.rs:113-119`：先 `available_to_fill` 问够不够，不够直接返回空成交；**够则降级成 IOC 走同一撮合路径** |

FOK 的实现值得单独看：它不是独立撮合逻辑，而是「预检 + 类型改写」。这个技巧让四种订单类型共享同一个 `match_incoming`，新增类型时不用动撮合核心——撮合核心是系统里最不该长分支的地方。

---

## 路由

多市场才需要：同一信号去 A 所还是 B 所，看手续费、队列、延迟。单市场 demo 路由 = 「只有一个下游 `submit`」。  
腿风险、跨所套利是 [14 §12.5](../14-hft-engineering/chapter-12-market-making-arbitrage/12.5-跨所套利与腿风险.md)，本模块不展开实现。

---

## HFT/嵌入式关联

- **状态机退化形态**是嵌入式常见手法：资源够、约束少时把状态机压到两个标志位，但要在注释里写明「哪三个状态被省略了、什么条件下必须补回来」。demo 的 `live_*_id` 注释就做了这件事。
- **「不变量由类型守卫」**（私有字段 + 公开方法）在 C 嵌入式里要靠约定和 review，Rust 里编译器强制——这是 Rust 进嵌入式最实际的卖点之一，比「零成本抽象」这种口号具体得多。
- **FOK = 预检 + 降级**的模式在驱动开发里同样适用：把「新特性」翻译成已有路径的参数组合，比在热路径里加分支便宜得多。

---

## 代码自测

<details>
<summary>1. demo 的 <code>live_bid_id</code> / <code>live_ask_id</code> 为什么只是「状态机的退化形态」？生产要补哪三个状态？</summary>

它只表达「无在途 / 有在途 id」两态。生产必须补：撤单在途（cancel 已发出未确认，期间可能成交）、部分成交（剩余量更新）、拒单（异步 reject 回报）。缺了这三个，就会重复挂单或撤已经没了的单。
</details>

<details>
<summary>2. STP 两道闸门分别在哪个文件哪几行？为什么策略侧有了还要撮合侧？</summary>

策略侧 `strategy.rs:135-137`（ask ≤ bid 时强制拉开 1 tick）；撮合侧 `book.rs:259-263`（跳过同 owner 挂单）。只留策略侧：库存 skew 大时计算 bug 仍可能产出交叉价；只留撮合侧：FOK 的可用量会把自家的单算进去。纵深防御，两层都要。
</details>

<details>
<summary>3. FOK 在 demo 里是怎么实现的？为什么不给 FOK 写独立撮合逻辑？</summary>

`book.rs:113-119`：先 `available_to_fill` 预检总量（已过滤自家单），不够返回空成交；够则把类型改写成 IOC 走共享的 `match_incoming`。四种类型共用一条撮合路径，新增类型不动撮合核心——撮合核心是最不该长分支的地方。
</details>

<details>
<summary>4. 撤单为什么先 <code>index.remove(&id)</code> 而不是先找价位队列？</summary>

`index` 是 O(1) 的「单在不在簿上」判据。单已成交/已撤时 index 里没有，直接 false 返回，不必扫任何队列。且 index 与簿内容必须严格同步（`book.rs:31` 注释），先动 index 再动队列，任何一步失败都不会留下「队列里有但索引里没有」的半一致状态。
</details>

---

## 卡住翻哪篇

| 卡住了… | 翻这里 |
|---------|--------|
| 订单类型 IOC/FOK | [14 §3.2](../14-hft-engineering/chapter-03-orderbook-depth-market-data/3.2-撮合引擎三种场景.md) · demo `cargo test` |
| FIFO 与改价去队尾 | [14 §3.3](../14-hft-engineering/chapter-03-orderbook-depth-market-data/3.3-同价匹配算法.md) |
| 网关出入 | [14 §2.1](../14-hft-engineering/chapter-02-exchange-architecture-matching/2.1-关键路径总览与网关.md) |
| 本模块撮合 | [`demo/src/book.rs`](./demo/src/book.rs) |
