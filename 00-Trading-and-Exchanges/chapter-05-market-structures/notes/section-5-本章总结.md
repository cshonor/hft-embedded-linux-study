## 本章总结

| 维度 | 影响 |
|------|------|
| 连续 vs 集合 | 策略时间粒度 |
| Quote vs Order vs Broker | 你在 ecosystem 中的角色 |
| Pre / Post transparency | 信息 edge 与 adverse selection |
| Routing + Order book | 系统架构与 latency 预算 |

```
市场结构
    → 谁提供流动性、谁索取
    → 谁看见什么、多快看见
    → 最终：流动性 · 成本 · 价格效率 · 哪些策略存活
```

> **HFT 读者 takeaway：** 第五章是 **交易所产品规格说明书** 的业务版——读 matching engine 代码前，先能回答：这是 order-driven 还是 quote-driven？full book 还是 top？continuous 还是 call？这些答案决定你的 Rust 引擎该实现哪些模块。

---
