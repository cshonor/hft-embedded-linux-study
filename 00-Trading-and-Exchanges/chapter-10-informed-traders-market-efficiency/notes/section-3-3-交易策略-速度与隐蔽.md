## 3. 交易策略：速度与隐蔽 (Speed vs Stealth)

知情者须在 **最小化价格冲击** 与 **最大化利润** 间权衡：

| 模式 | 何时 | 行为 |
|------|------|------|
| **激进交易 (Aggressive)** | 私人信息 **即将公开**；或 **许多知情者** 将同时行动 | **快速、大量吃单** — 抢在他人前 |
| **隐蔽交易 (Stealth / Stealth trading)** | **独家优势** 且 **短期不会丧失** | **缓慢、拆单、隐藏意图** — 防 LP **撤单或抬价** |

| 权衡维度 | 激进 | 隐蔽 |
|----------|------|------|
| **速度** | 高 | 低 |
| **Market impact** | 高 | 低 |
| **被 front-run 风险** | 信息窗口竞争 | 被 [Ch 11 指令预期者](../chapter-11-order-anticipators/) 嗅探 |
| **工具** | Market / marketable limit | Iceberg、VWAP/TWAP、dark pool |

| HFT 视角 |
|----------|
| **Latency race** 多发生在 **aggressive informed** 场景（新闻、并购、FOMC） |
| **Execution algos + hidden liquidity** = stealth 工程化 |
| Maker 须建模：**incoming order 的 informed probability** → 动态 **widen / pull** |

---
