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
| **Harris** | [Ch 4 Orders](../chapter-04-交易指令与订单类型.md) · [Ch 5 Market Structures](../chapter-05-市场结构.md) |
| **笔记** | [notes/milestone-01-订单类型与LOB/](./notes/milestone-01-订单类型与LOB/) |
| **验收** | 单元测试：插入限价单后 best bid/ask 正确；市价单「吃」最优档 |

---

## M2 · 撮合引擎（价格–时间优先）

| 项 | 内容 |
|----|------|
| **目标** | `Match()`：限价挂单入簿；可成交则按 **价格优先、同价时间优先** 生成 `Trade` |
| **Harris** | [Ch 6 Order-driven Markets](../chapter-06-指令驱动市场.md) |
| **笔记** | [notes/milestone-02-撮合引擎/](./notes/milestone-02-撮合引擎/) |
| **验收** | 回放固定订单序列，成交列表与手工推演一致 |

---

## M3 · 价差与流动性指标

| 项 | 内容 |
|----|------|
| **目标** | 实时 **spread**、档位深度、简单 **implementation shortfall** 统计 |
| **Harris** | [Ch 13 Dealers](../chapter-13-做市商.md) · [Ch 14 Bid-Ask Spreads](../chapter-14-买卖价差.md) · [Ch 19 Liquidity](../chapter-19-流动性.md) |
| **笔记** | [notes/milestone-03-价差与流动性/](./notes/milestone-03-价差与流动性/) |
| **验收** | 对同一 LOB 快照，spread = best_ask − best_bid |

---

## M4 · 服务层与「DEX」形态（可选扩展）

| 项 | 内容 |
|----|------|
| **目标** | HTTP/JSON 或 WebSocket：`POST /order`、`GET /book`；多 `symbol` |
| **Harris** | [Ch 25](../chapter-25-内部化优先撮合与交叉交易.md)–[27](../chapter-27-场内交易与自动交易系统.md)（电子化、多 venue 直觉） |
| **笔记** | [notes/milestone-04-API与多交易对/](./notes/milestone-04-API与多交易对/) |
| **验收** | 本地 curl 下单、查簿；QEMU 式「能跑通一条链路」即可 |

---

## 读理论时的复盘 checklist

- [ ] 刚读完的 **chapter-XX** 在本表哪一行？
- [ ] 对应 **milestone** 的 `notes/` 里有没有「书上概念 → 我的类型/函数」对照？
- [ ] `code/` 里能否 **单测或 main 演示** 该概念？

---

← [00-practice-go-dex 导读](./README.md) · [Harris 全书 OUTLINE](../OUTLINE.md)
