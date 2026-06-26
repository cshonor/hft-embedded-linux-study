## 4. 买方防御策略 (Defensive Strategies)

防 **寄生型剥削** 的三类策略：

### 4.1 回避 / 隐蔽 (Evasive Strategies)

| 手段 | 说明 |
|------|------|
| **多经纪人** | 无人知 **真实总规模** |
| **完全匿名系统** | 不显示身份 / 订单簿 |
| **拆单分批 (Split orders)** | 降低 **单次 footprint** |
| **未公开 / 冰山指令 (Undisclosed / Iceberg)** | 隐藏 **真实 size** |

| HFT 视角 |
|----------|
| **VWAP/TWAP/IS**、**dark pool**、**SOR** — 工程化 evasive |
| **Child order randomization** 防 **detection** |

### 4.2 欺骗 / 迷惑 (Deceptive Strategies)

| 手段 | 说明 |
|------|------|
| **反向小额公开交易** | 制造假象 |
| **虚假订单指示 (Order indications)** | 转移注意力 |
| **谎报规模** | 误导对手 **真实 intent** |

| HFT 视角 |
|----------|
| 与 [Ch 12 虚张声势](../chapter-12-bluffers-market-manipulation/) **边界模糊** — buy-side **合法迷惑** vs **操纵** |
| **Decoy orders** — 监管敏感 |

### 4.3 进攻 / 反击 (Offensive Strategies)

| 典型：**诱捕 / 设局 (Sting)** | 说明 |
|------------------------------|------|
| **做法** | **真实意图反方向** 挂单 → 引诱 **抢先者** 入场 |
| **触发** | 抢先者抢跑时 **立刻成交** + **撤销虚假单** → 让抢先者 **承担损失** |

→ 与 Ch 11 **攻防反转**

| HFT 视角 |
|----------|
| **Anti-gaming** 是 buy-side 与 sell-side HFT **军备竞赛** |
| **Spoofing 指控** 风险 — sting 须在 **合规框架** 内 |

---
