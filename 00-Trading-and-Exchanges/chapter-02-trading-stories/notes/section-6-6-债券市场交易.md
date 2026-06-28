# Ch 2 §6 债券市场交易 · A Bond Market Trade

> **Trading and Exchanges** · Larry Harris · **精读**

## 6. 债券市场交易 (A Bond Market Trade)

Sam 购买 **$5000 万** 长期公司债：

1. **OTC 场外** — **不在** 集中交易所挂公开单
2. 电话 **所罗门兄弟 (Salomon Brothers) 销售交易员**
3. 以 **10 年期国债收益率** 为基准 **讨价还价**（spread over Treasury）
4. 成交后 **NSCC** 电子记账，**货银对付 (DVP)**

---

## 为何债券典型走 OTC？

| 原因 | 说明 |
|------|------|
| **标准化程度低** | 每家公司债 **条款不同**（期限、息票、赎回、担保）— 难进 **单一 LOB** |
| **单笔量大** | Sam **$5000 万** — 公开簿会 **严重冲击** 薄流动性 |
| **流动性分散** | 靠 **经销商库存 + 询价 (RFQ)**，不是连续竞价 |
| **灵活议价** | 价 = **基准利率 + 信用利差 + 量折扣** — 一对一谈 |

→ 与 [Ch 3 §2.4 OTC](../../chapter-03-trading-industry/notes/section-2-2-交易促成机构.md#24-otc场外交易--over-the-counter) 定义一致：**经销商询价、谈价谈量、单独结算**。

---

## OTC 债券 vs 交易所股票

```
Sam 买债（OTC）                    Jennifer 买股票（交易所）
────────────────                   ────────────────────────
  找 dealer 询价                      App 下单 → 公开 LOB
  谈 yield / spread                   规则 Match()
  无公开 BBO                          Level I 全市场可见
  对手方信用风险                      交易所/CCP 机制（视产品）
```

---

## HFT 关联

| | |
|---|---|
| **公司债** | 纯 LOB 式 HFT **少** — 主路径是 **dealer RFQ**、电子 OTC 平台 |
| **国债 / 利率** | **期货 (CBOT/CME)**、电子化 **cash treasury** — 更接近 HFT 主战场 |
| **与暗池** | 债券 **几乎全 OTC**；股票大块可选 **暗池** 或 **Upstairs OTC** — 见 [§3](./section-3-3-超大宗股票抛售.md) |

> **Takeaway：** 债券故事 = **OTC 范式样板**；练 go-dex **公开簿** 的同时，知道 **大量真实成交量** 发生在 **交易所之外的议价市场**。

---

← [§5 期权](./section-5-5-期权市场交易.md) · [Ch 2 README](../README.md) · 下一节 [§7 外汇](./section-7-7-外汇市场交易.md) · [§8 总结](./section-8-本章总结-七个故事-全书概念地图.md)

**OTC 总论：** [Ch 3 §2.4](../../chapter-03-trading-industry/notes/section-2-2-交易促成机构.md#24-otc场外交易--over-the-counter)
