# 第9章 风险控制与仓位管理

> 策略不可信。风控是另一条代码路径，几次 `if`，失败就地变成拒单。

← [第8章](./chapter-08-订单管理与路由系统.md) · 下一章：[第10章 监控](./chapter-10-系统监控与性能调优.md)

---

## 本地拒单链（热路径）

和 [14 §10.1](../14-hft-engineering/chapter-10-risk-compliance-slippage/10.1-本地拒单链.md)、P10 `RiskGate` 同一张表：

| 检查 | 防什么 |
|------|--------|
| 价格带 vs mid | 乌龙指打穿盘口 |
| 单笔数量上限 | 巨量单 |
| 持仓上限 | 库存滚雪球；**减仓单仍放行** |
| 每拍下单次数 | 流速 / 交易所断开 |
| kill switch | 总闸 |

全部检查无分配、无锁、无系统调用。参数由冷线程更新（原子换指针 / 拷一份小 struct）。

---

## 仓位

`inventory`：买加卖减。标记 PnL = `cash + inventory * mid`。  
风控看的是「这张单成交后绝对仓位会不会更大」——所以已经超限时只许反向单。

自成交（STP）：同一 `owner` 的挂单互不成交。P10 / demo 都做了；监管视角这是底线。

---

## Rust 落点

```rust
fn check(o: &Order, book: &Book, inv: Qty) -> Result<(), Reject>;
```

`Err` 只在引擎里变成计数器 + 异步日志，不要 `unwrap`。测试要覆盖：加仓拒绝、减仓通过（P10 `risk-pos-*`）。

---

## 卡住翻哪篇

| 卡住了… | 翻这里 |
|---------|--------|
| P10 实现 | [risk.hpp](../projects/P10-hft-prototype/part-a-demo/src/risk.hpp) |
| 降级 / 快市 | [14 §10.4](../14-hft-engineering/chapter-10-risk-compliance-slippage/10.4-风控降级.md) |
| 本模块代码 | [`demo/`](./demo/) `RiskGate` |
