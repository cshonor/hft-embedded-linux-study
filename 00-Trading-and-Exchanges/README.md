# Trading and Exchanges — Larry Harris

**文件夹 00 · 阶段 0（建议最先读）** · 全书 **29 章 / 7 部分**

📋 **完整目录与 HFT 读/跳标注** → [OUTLINE.md](./OUTLINE.md)

**配套练手（Go DEX · 与理论绑定、代码独立）：** [00-practice-go-dex/](./00-practice-go-dex/) — 理论读各章 `chapter-XX-*/`，实现见 `00-practice-go-dex/notes/` + `code/`。

---

## 全书结构

```
chapter-XX-english-slug/   ← 全书 29 章均已采用
├── README.md
└── notes/section-*.md
```

### 引言
| 章 | 笔记 |
|----|------|
| 1 | [chapter-01-introduction-market-microstructure](./chapter-01-introduction-market-microstructure/) |
| 2 | [chapter-02-trading-stories](./chapter-02-trading-stories/) |

### Part I · 交易的结构
| 章 | 笔记 |
|----|------|
| 3 | [chapter-03-trading-industry](./chapter-03-trading-industry/) |
| 4 | [chapter-04-orders-and-order-types](./chapter-04-orders-and-order-types/) |
| 5 | [chapter-05-market-structures](./chapter-05-market-structures/) |
| 6 | [chapter-06-order-driven-markets](./chapter-06-order-driven-markets/) |
| 7 | [chapter-07-brokers](./chapter-07-brokers/) |

### Part II · 交易的益处
| 章 | 笔记 |
|----|------|
| 8 | [chapter-08-why-people-trade](./chapter-08-why-people-trade/) |
| 9 | [chapter-09-good-markets](./chapter-09-good-markets/) |

### Part III · 投机者
| 章 | 笔记 |
|----|------|
| 10 | [chapter-10-informed-traders-market-efficiency](./chapter-10-informed-traders-market-efficiency/) |
| 11 | [chapter-11-order-anticipators](./chapter-11-order-anticipators/) |
| 12 | [chapter-12-bluffers-market-manipulation](./chapter-12-bluffers-market-manipulation/) |

### Part IV · 流动性提供者
| 章 | 笔记 |
|----|------|
| 13 | [chapter-13-dealers](./chapter-13-dealers/) |
| 14 | [chapter-14-bid-ask-spreads](./chapter-14-bid-ask-spreads/) |
| 15 | [chapter-15-block-traders](./chapter-15-block-traders/) |
| 16 | [chapter-16-value-traders](./chapter-16-value-traders/) |
| 17 | [chapter-17-arbitrageurs](./chapter-17-arbitrageurs/) |
| 18 | [chapter-18-buy-side-traders](./chapter-18-buy-side-traders/) |

### Part V · 流动性与波动性
| 章 | 笔记 |
|----|------|
| 19 | [chapter-19-liquidity](./chapter-19-liquidity/) |
| 20 | [chapter-20-volatility](./chapter-20-volatility/) |

### Part VI · 评估与预测
| 章 | 笔记 |
|----|------|
| 21 | [chapter-21-transaction-cost-measurement](./chapter-21-transaction-cost-measurement/) |
| 22 | [chapter-22-performance-evaluation-prediction](./chapter-22-performance-evaluation-prediction/) |

### Part VII · 市场结构
| 章 | 笔记 |
|----|------|
| 23 | [chapter-23-index-portfolio-markets](./chapter-23-index-portfolio-markets/) |
| 24 | [chapter-24-specialists](./chapter-24-specialists/) |
| 25 | [chapter-25-internalization-preferencing-crossing](./chapter-25-internalization-preferencing-crossing/) |
| 26 | [chapter-26-competition-within-among-markets](./chapter-26-competition-within-among-markets/) |
| 27 | [chapter-27-floor-vs-automated-trading](./chapter-27-floor-vs-automated-trading/) |
| 28 | [chapter-28-bubbles-crashes-circuit-breakers](./chapter-28-bubbles-crashes-circuit-breakers/) |
| 29 | [chapter-29-insider-trading](./chapter-29-insider-trading/) |

---

## HFT 精读捷径

```
Ch 1–6   结构 + 指令驱动市场（LOB / 撮合规则）
Ch 10–11 知情 · 指令预期（adverse selection）
Ch 13–14 做市商 · 价差
Ch 17    套利 · 多市场连接
Ch 19    流动性四维
Ch 21    TCA / markout
Ch 25–27 PFOF · 碎片化 · 电子化
```

完整路线 → [LEARNING-CHAIN.md](../LEARNING-CHAIN.md) · [HFT-READING-ROADMAP.md](../HFT-READING-ROADMAP.md)
