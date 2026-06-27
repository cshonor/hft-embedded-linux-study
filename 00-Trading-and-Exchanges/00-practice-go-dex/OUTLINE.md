# Go DEX 练手 · 里程碑与 Harris 章节对照

> **原则：** 理论按 **原书章节** 读（`../chapter-*.md`）；实现按 **本表里程碑** 推进（`notes/` + `code/`）。

| 状态 | 说明 |
|------|------|
| 🔴 | 当前优先 |
| 🟡 | 下一阶段 |
| ⚪ |  backlog |

---

## M1 · 订单与 LOB 数据结构

| 项 | 内容 |
|----|------|
| **目标** | `Order`（side, type, price, qty, time）；`OrderBook`（bid/ask 两侧价位队列） |
| **Harris** | [Ch 4 Orders](../chapter-04-orders-and-order-types/) · [Ch 5 Market Structures](../chapter-05-market-structures/) |
| **笔记** | [notes/milestone-01-订单类型与LOB/](./notes/milestone-01-订单类型与LOB/) |
| **验收** | 单元测试：插入限价单后 best bid/ask 正确；市价单「吃」最优档 |

---

## M2 · 撮合引擎（价格–时间优先）

| 项 | 内容 |
|----|------|
| **目标** | `Match()`：限价挂单入簿；可成交则按 **价格优先、同价时间优先** 生成 `Trade` |
| **Harris** | [Ch 6 Order-driven Markets](../chapter-06-order-driven-markets/) |
| **笔记** | [notes/milestone-02-撮合引擎/](./notes/milestone-02-撮合引擎/) |
| **验收** | 回放固定订单序列，成交列表与手工推演一致 |

---

## M3 · 价差与流动性指标

| 项 | 内容 |
|----|------|
| **目标** | 实时 **spread**、档位深度、简单 **implementation shortfall** 统计 |
| **Harris** | [Ch 13 Dealers](../chapter-13-dealers/) · [Ch 14 Bid-Ask Spreads](../chapter-14-bid-ask-spreads/) · [Ch 19 Liquidity](../chapter-19-liquidity/) |
| **笔记** | [notes/milestone-03-价差与流动性/](./notes/milestone-03-价差与流动性/) |
| **验收** | 对同一 LOB 快照，spread = best_ask − best_bid |

---

## M4 · 服务层与「DEX」形态（可选扩展）

| 项 | 内容 |
|----|------|
| **目标** | HTTP/JSON 或 WebSocket：`POST /order`、`GET /book`；**Level 1** `GET /ticker`（best bid/ask）；多 `symbol` |
| **Harris** | [Ch 25](../chapter-25-internalization-preferencing-crossing/)–[27](../chapter-27-floor-vs-automated-trading/)（电子化、多 venue 直觉） |
| **笔记** | [notes/milestone-04-API与多交易对/](./notes/milestone-04-API与多交易对/) |
| **验收** | 本地 curl 下单、查簿；**ticker 卖价 = min(Asks)**、买价 = max(Bids)，与手工一致 |

---

## M5 · 机构级扩展（backlog）

| 项 | 内容 |
|----|------|
| **前提** | M2 公开簿 `Match()` 已正确；M4 多 symbol / 推送可选 |
| **目标** | **暗池**、**冰山单**、**多 ingress**、**多层流动性路由 (SOR)**（MM → ECN → 下一 venue）、**风控**、**算法拆单**（parent → child orders） |
| **Harris** | [Ch 4](../chapter-04-orders-and-order-types/) 隐藏量 · [Ch 25](../chapter-25-internalization-preferencing-crossing/) crossing · [Ch 27](../chapter-27-floor-vs-automated-trading/) FIX/电子化 |
| **理论** | [Ch 2 §2 机构 vs 散户](../chapter-02-trading-stories/notes/section-2-2-机构股票交易.md#与-go-dex你现在写的是散户级现货交易所) |
| **验收** | 冰山单对外深度 ≠ 真实可成交量；暗池成交 **不出现在** 公开 LOB；**SOR**：第一层流动性不足时自动路由下一 venue；拆单 impact 低于单笔市价 |

> **定位：** M1–M4 = **散户级现货交易所**；M5 = 向 **专业机构级平台** 进阶（热路径仍建议 Rust/C++，Go 做原型与外围）。

---

## 读理论时的复盘 checklist

- [ ] 刚读完的 **chapter-XX** 在本表哪一行？
- [ ] 对应 **milestone** 的 `notes/` 里有没有「书上概念 → 我的类型/函数」对照？
- [ ] `code/` 里能否 **单测或 main 演示** 该概念？

---

← [00-practice-go-dex 导读](./README.md) · [Harris 全书 OUTLINE](../OUTLINE.md)
