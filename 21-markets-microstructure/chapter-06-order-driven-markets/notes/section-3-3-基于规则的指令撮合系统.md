## 3. 基于规则的指令撮合系统 (Rule-Based Order-Matching)

绝大多数 **ECN / 电子化交易所** 采用此类系统：交易者提交限价单，**预设层级规则** 自动撮合。

在 **价格优先** 前提下，常见 **次要优先规则**：

| 规则 | 英文 | 要点 |
|------|------|------|
| **时间优先** | Time precedence | 同价 FIFO；最常见 |
| **展示优先** | Display precedence | **可见单** 优于 **隐藏单**，鼓励公开意图 |
| **规模/数量优先** | Size precedence | 按比例分配 (pro-rata) 或大单优先——因市场而异 |

| HFT 视角 |
|----------|
| **Display vs hidden** → iceberg、reserve size、dark pool 路由策略 |
| **Pro-rata**（部分期货）vs **FIFO**（多数 equity）→ queue position 估值完全不同 |
| 引擎须实现：**price → time / display / size** 的 **确定性排序**，否则 replay 对不上交易所 |

---
