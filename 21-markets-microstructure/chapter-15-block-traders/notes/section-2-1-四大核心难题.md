## 1. 四大核心难题 (Block Trading Problems)

### 1.1 潜在需求问题 (Latent Demand Problem)

| 问题 | 许多愿交易者 **不在市场中** |
|------|---------------------------|
| **原因** | 管理限价单 **成本高**；或 **尚未意识到** 交易机会 |
| **名称** | **潜在需求 (Latent demand)** — 隐藏的 **响应型交易者 (Responsive traders)** |
| **任务** | 大宗方须 **主动发现** 这些隐藏对手 |

| HFT 视角 |
|----------|
| **Indication of Interest (IOI)**、**dark pool matching**、**broker capital introduction** |
| 电子时代：**conditional orders**、**pinging dark** — 仍是在挖 latent demand |

### 1.2 指令暴露问题 (Order Exposure Problem)

| 恐惧 | 广泛公开意图 → **「破坏市场」(Spoiling the market)** |
|------|------------------------------------------------------|
| **后果** | [Ch 11 抢先交易者](../chapter-11-order-anticipators/) **提前入场** |
| | **同向** 交易者 **加速** |
| | **反向** 交易者 **推迟** |
| **结果** | **价格冲击极大加速** |

| HFT 视角 |
|----------|
| **Stealth execution**、**iceberg**、**minimize footprint** — Ch 10 隐蔽交易的大单版 |
| **Flow sniffing / prediction** 是 exposure 的 **现代威胁** |

### 1.3 价格歧视问题 (Price Discrimination Problem)

| LP 恐惧 | 发起人 **隐瞒真实总规模**，**拆分吃流动性** |
|---------|---------------------------------------------|
| **手段** | 先用 **好价** 成交一部分，再用 **差价** 吃剩余 — **价格歧视** |
| **LP 要求** | 发起人须 **可信展示全部规模** |
| **匿名市场难点** | **撒谎无声誉成本** → 难信任 |

| HFT 视角 |
|----------|
| **Show full size or pre-negotiated block** vs **algo slicing** 张力 |
| **Last-look / firm-up** 机制 — 确认规模后报价 |

### 1.4 信息不对称问题 (Asymmetric Information Problem)

| LP 恐惧 | 发起人是掌握基本面的 **知情交易者 (Informed)** |
|---------|-----------------------------------------------|
| **后果** | 做对手盘 → **严重逆向选择**（[Ch 14](../chapter-14-bid-ask-spreads/)） |

| HFT 视角 |
|----------|
| Block LP 要价含 **巨大 adverse selection premium** |
| **Buy-side TCA** — 证明 **utilitarian motive** 可降低 **冲击成本** |

---
