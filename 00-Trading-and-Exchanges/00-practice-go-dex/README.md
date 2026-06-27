# 00-practice-go-dex · Harris 配套练手

> **父模块：** [00-Trading-and-Exchanges](../README.md)（Larry Harris · *Trading and Exchanges*）  
> **定位：** 用 **Go** 写一个最小 **DEX / 指令驱动撮合** 练手项目 — **理论归理论、代码归代码**，复盘时一眼对上 Harris 章节。交易所 = **金融产品大卖场**；go-dex = 先造 **现货区的基础收银台**（通用 LOB 撮合），衍生品区的保证金/行权/爆仓以后 **叠在引擎外层** — 见 [Ch 1 §3 · 大卖场比喻](../chapter-01-introduction-market-microstructure/notes/section-3-3-交易工具与市场.md#三主流交易所--金融产品大卖场)。**M1–M4 覆盖散户级公开簿**；机构暗池/冰山/FIX/拆单 → [Ch 2 §2](../chapter-02-trading-stories/notes/section-2-2-机构股票交易.md#与-go-dex你现在写的是散户级现货交易所) · [M5](./OUTLINE.md#m5--机构级扩展-backlog)。  
> **边界：** 本项目是 **市场基础设施**（稳定、公平、全量撮合），不是 HFT 抢跑系统 — 见 [Ch 1 §3](../chapter-01-introduction-market-microstructure/notes/section-3-3-交易工具与市场.md#五同一套订单簿两种完全不同的系统)。  
> **语言路线：** **Go = 原型**（先把 `Match()` 逻辑跑通）；行业里 **HFT 网关 / 核心热路径用 Rust 或 C++**（无 GC 抖动），Go 更适合回测、风控、监控等外围 — 见 [Ch 1 §4.4 · 技术栈分工](../chapter-01-introduction-market-microstructure/notes/section-4-4-贯穿全书的关键主题.md#hft-里谁用什么语言行业分工)。

---

## 为什么单独一个子目录？

| | **上层 `00-Trading-and-Exchanges/`** | **本目录 `00-practice-go-dex/`** |
|---|--------------------------------------|----------------------------------|
| 内容 | **按原书章节** 的读书笔记（Ch 1–29 平铺） | **练手项目** 的设计笔记、里程碑、**Go 源码** |
| 排序 | 与 Harris **目录顺序一致** | 与 **实现进度** 一致（订单 → LOB → 撮合…） |
| 模块边界 | 市场微观 **业务语言** | 与 `09` 自制 OS、`05` Linux 内核等 **系统模块清晰分开** |

读 [chapter-04](../chapter-04-orders-and-order-types/) 的限价/市价单 → 来 [notes/milestone-01-订单类型与LOB](./notes/milestone-01-订单类型与LOB/) 看自己写的结构体与测试；读 Ch 6 指令驱动市场 → 对 [milestone-02](./notes/milestone-02-撮合引擎/) 的 match 逻辑 — **复盘路径顺**。

---

## 目录结构

```
00-practice-go-dex/
├── README.md          ← 本页
├── OUTLINE.md         ← 里程碑与 Harris 章节对照
├── notes/             ← 实践笔记（设计、复盘、与理论章节的链接）
│   └── milestone-XX-…/
└── code/              ← Go 工程（独立 go module）
```

**理论笔记不要搬进 `notes/`** — 仍写在 `../chapter-XX-….md`；这里只写 **「我实现了什么、对应书上哪一节」**。

---

## 里程碑概览

| 阶段 | 实践目标 | 对照 Harris |
|------|----------|-------------|
| **M1** | 订单类型、限价簿数据结构 | Ch [4](../chapter-04-orders-and-order-types/) · [5](../chapter-05-market-structures/) |
| **M2** | 价格–时间优先撮合、部分成交 | Ch [6](../chapter-06-order-driven-markets/) |
| **M3** | 价差、流动性、maker/taker 视角 | Ch [13](../chapter-13-dealers/) · [14](../chapter-14-bid-ask-spreads/) · [19](../chapter-19-liquidity/) |
| **M4+** | 多交易对、HTTP/WS API、简单 DEX 前端（可选） | Ch [25](../chapter-25-internalization-preferencing-crossing/)–[27](../chapter-27-floor-vs-automated-trading/) |
| **M5** | 机构级：暗池、冰山、多 ingress、**OTC/upstairs 协商成交**、SOR、风控、拆单（backlog） | [Ch 2 §2–§3](../chapter-02-trading-stories/notes/) |

完整表 → [OUTLINE.md](./OUTLINE.md)

---

## 快速入口

| 用途 | 链接 |
|------|------|
| **29 章 ↔ 代码索引** | [code/HARRIS-INDEX.md](./code/HARRIS-INDEX.md) |
| 实践笔记索引 | [notes/](./notes/) |
| Go 源码 | [code/](./code/) |
| 全书理论目录 | [../OUTLINE.md](../OUTLINE.md) |
| HFT 精读捷径 | [../README.md#hft-精读捷径29-章中优先](../README.md) |

---

## 与仓库其他模块的关系

```
00 Harris 理论（chapter-*.md）
    ↓ 配套实践
00-practice-go-dex（本目录 · 业务侧 LOB/DEX）

09 自制 OS / 05 LKD …（系统与内核 · 不同大模块，互不混放）
    ↓ 后期
15 HFT 工程 · 16 Rust 量化（网关/撮合热路径 → Rust；Go 留外围；业务模型仍来自 00）
```

返回 → [00 导读](../README.md) · [总路线 ../../HFT-READING-ROADMAP.md](../../HFT-READING-ROADMAP.md)
