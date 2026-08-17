# 第4章 时间序列与 K 线处理

> Bar 是给慢策略看的压缩视图。HFT 热路径看的是每一笔 tick / 每一档 book。

← [第3章](./chapter-03-行情数据采集与清洗.md) · 下一章：[第5章 策略](./chapter-05-量化策略模型开发.md)

---

## 两套时钟

| | Tick / 逐笔 | K 线（OHLCV） |
|--|-------------|---------------|
| 谁用 | 做市、吃单、队列位置 | 日频、因子、研究 |
| 更新 | 每条行情 | 按 1m / 5m / 1d 收口 |
| 陷阱 | 乱序、重复 | **未来函数**：用了还没走完的这根 K 线的收盘价 |

回测若用「当日收盘」下「当日开盘」的单，结果会好看、实盘会崩。规则：只允许用 **已经关闭** 的 bar。

---

## 聚合

从 tick 合成 1 分钟 K：

- `open` = 窗口第一笔  
- `high` / `low` = 窗口极值  
- `close` = 窗口最后一笔  
- `volume` = 窗口成交量之和  

用环形数组存最近 N 根，不要每个 tick `Vec::insert(0, …)`。

HFT 引擎里 **不要** 每 tick 算一遍 20 日均线——那是冷路径或独立研究进程。热路径最多看 BBO、盘口不平衡、短窗口波动。

---

## Rust 落点

```rust
struct Bar {
    open: Price,
    high: Price,
    low: Price,
    close: Price,
    volume: Qty,
    start_ts: i64,
}
```

价格字段继续用 `i64` tick。均线可以 `i64` 累加再除，注意除法截断；研究图再用 float。

---

## 卡住翻哪篇

| 卡住了… | 翻这里 |
|---------|--------|
| 做市并不靠 K 线 | [14 §12.1](../14-hft-engineering/chapter-12-market-making-arbitrage/12.1-做市核心等式与逆向选择.md) |
| P10 用的是 tick 回放 | [P10 design](../projects/P10-hft-prototype/docs/design.md) |
