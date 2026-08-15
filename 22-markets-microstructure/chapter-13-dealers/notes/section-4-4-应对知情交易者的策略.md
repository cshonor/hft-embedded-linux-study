## 4. 应对知情交易者的策略

做市商 **难直接识别** 知情者 → 须从 **订单流推断** 基本面变化，并采取防御：

### 4.1 迅速调整报价

若怀疑刚 **买入了知情者的抛售**：

| 动作 | 目的 |
|------|------|
| **立即降低双边报价** | 阻止其他知情者继续 **卖给你** |
| | **降价吸引不知情买家** — 在 **真跌前** 清空库存 |

| HFT 视角 |
|----------|
| **Microsecond quote update** after fill；**cancel-replace storm** |
| **Auto-hedge** 到 futures / ETF |

### 4.2 拉宽价差：逆向选择价差成分

将 **遇到知情者的概率** 与 **预期损失** **预埋** 进报价 → ask **提高**、bid **降低**。

**额外加宽部分** = **逆向选择价差成分 (Adverse selection spread component)**。

→ [Ch 14](../chapter-14-bid-ask-spreads/) 完整分解（inventory + adverse selection + monopoly 等）

| HFT 视角 |
|----------|
| **Widen spread when toxicity ↑**；**vol regime** 联动 |
| **Maker rebate** 须 **覆盖** expected adverse selection |

### 4.3 提防大单与匿名交易

| 信念 | 行为 |
|------|------|
| **大单更可能知情** | 报 **极宽价差** 或 **只显示小 size** |
| **匿名 = 隐藏身份** | **拒绝或回避** — 防 informed **stealth** |

| HFT 视角 |
|----------|
| **Size skew**、**max fill size**、**ISO / directed** 规则 |
| **Dark pool** 与 **lit market** 的 **adverse selection 不对称** |

---
