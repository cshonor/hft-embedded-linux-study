## 6. 本章总结

| 要点 | 含义 |
|------|------|
| **规则不是背景** | 决定 **权力、特权、盈余分配** |
| **优先规则** | Price → Time / Display / Size；即 **queue 逻辑** |
| **定价规则** | Uniform vs Discriminatory vs Derivative；即 **成交价函数** |
| **选市场 = 选规则** | 同一订单流在不同结构下 **成交量、成本、被 picked off 概率** 不同 |

> **HFT 读者 takeaway：** 读 matching engine 或写 Go/Rust 撮合前，先答四个问题——**(1)** continuous 还是 call？**(2)** 歧视性还是统一定价？**(3)** 次要优先是 time、display 还是 pro-rata？**(4)** 有没有衍生定价的交叉子系统？  
> 答案即 **spec**；Ch 4 的订单类型 + 本章规则 = 完整 LOB 语义。

---
