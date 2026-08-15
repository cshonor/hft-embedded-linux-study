## 6. 本章总结

| 要点 | 含义 |
|------|------|
| **定义** | 相对价值；买便宜卖贵，赌 **收敛** |
| **工具** | Hedge portfolio · basis · arbitrage spread vs band |
| **Pure vs Risk arb** | 均值回归有机制 vs 非平稳但短期回归 |
| **四风险** | 执行 · 基差/杠杆 · 模型 · 持有成本 |
| **角色** | Disciplinarian + Porter of liquidity |
| **vs Dealer** | 空间 vs 时间；共同维护 **价格一致性** |

> **HFT 读者 takeaway：** 多腿策略 **第一风险是 implementation（冰块探险者）**，第二风险是 **basis blowout + 杠杆（LTCM）**。`orderbook.go` 单所撮合只是 **一腿** — 真 arb 是 **跨 book 的 band 监控 + 同步 execution**。Ch 10–17 **知情分册** 收束；下一章 [Ch 18 买方交易者](../chapter-18-buy-side-traders/) 转向 **机构 flow** 视角。

---
