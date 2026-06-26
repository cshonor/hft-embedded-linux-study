## 1. 散户股票交易 (Retail Equity Trades)

### NYSE 上市股票

散户 Jennifer 购买 200 股 AT&T：

1. 经纪人查看 **综合报价系统**（最优买卖价 BBO）
2. 可选 **市价单 (Market order)** 或 **限价单 (Limit order)**
3. 订单经 **SuperDot** 路由至 NYSE 大厅 **专家 (Specialist / 做市商)**
4. 在 **电子限价指令簿** 中等待撮合，或由专家自行促成

| 概念 | HFT 关联 |
|------|----------|
| BBO / 限价簿 | 现代 LOB 的原型；HFT 竞争 queue position |
| Specialist | 演化 today's designated market maker (DMM) |
| SuperDot | 早期电子路由 →  today's direct market access |

### Nasdaq 股票

Jennifer 购买微软 (Nasdaq)：

1. 查看 **Nasdaq Level II**——多做市商 + ECN 分层报价
2. 订单路由至特定做市商（可能含 **订单流返点 payment for order flow**）
3. 或通过 **SuperSOES** 等自动执行系统成交

| 概念 | HFT 关联 |
|------|----------|
| Level II 深度 | 订单簿多档报价；HFT 读 full book |
| ECN | 与交易所竞争的电子化 venue |
| 订单流返点 | 零售流 vs 机构流；HFT 通常走 DMA 不经此路径 |

---
