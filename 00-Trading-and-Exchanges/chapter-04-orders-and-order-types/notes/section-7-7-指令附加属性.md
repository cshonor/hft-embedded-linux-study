## 7. 指令附加属性 (Order Instructions)

### 7.1 有效性与到期 (Validity / Expiration)

| 类型 | 说明 | HFT |
|------|------|-----|
| **Day** | 当日有效 | 默认 |
| **GTC** | 撤销前有效 | 长期挂单 |
| **IOC** | 立即成交否则撤销 | **HFT 常用**——不吃单不留痕 |
| **FOK** | 全部成交否则撤销 | 避免部分成交 |

> IOC/FOK 减少向市场 **赠送免费期权** 的时间

### 7.2 数量说明 (Quantity)

- **AON (All-or-none)**：全部成交或不成交——避免多次小单 **结算固定成本**
- **Minimum quantity**：最低成交量要求

### 7.3 显示说明 (Display / Hidden)

| 类型 | 目的 |
|------|------|
| **Hidden / Undisclosed** | 隐藏规模，防 **意图暴露** |
| **Iceberg 冰山** | 只展示一小部分数量 |

| HFT 视角 |
|----------|
| LOB 可见深度 ≠ 真实深度；解析 **hidden liquidity** 是 advanced LOB 技能 |
| 自身策略常用 iceberg 防被 front-run |

---
