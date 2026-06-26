## 5. 价格变动敏感指令 (Tick-Sensitive Orders)

### 定义

执行条件取决于 **上一笔价格变动方向**：

- **Buy downtick**：仅下跌或平盘下跌时可买
- **Sell uptick**：仅上涨或平盘上涨时可卖

### 特点

- 动态限价，避免 **冲击市场**，倾向 **提供流动性**
- **十进制化 (Decimalization)** 后 tick 变小 → 吸引力 **大幅下降**

| HFT 视角 |
|----------|
| 历史 NYSE 规则；现代 tick size  debate (penny vs sub-penny) 影响 HFT 粒度 |

---
