## 4. 市场信息与路由系统

### 4.1 指令路由系统 (Order-routing systems)

在客户、broker、dealer、exchange 间传递：

- 新单、撤单
- **Execution reports（成交回报）**

**速度与准确性** → 能否抓住转瞬即逝的机会。

| HFT 视角 |
|----------|
| **OMS/EMS → exchange gateway → matching engine** 全链路 latency |
| 错单、慢回报 = 风险；co-located **binary protocol** (FIX/native) |

### 4.2 指令簿 (Order books)

保存尚未成交订单（限价单、止损单等）——电子库或历史纸质簿。

| HFT 视角 |
|----------|
| **LOB = HFT 核心数据结构**；详见 [Ch 6 指令驱动市场](../chapter-06-order-driven-markets/) |
| 多 venue → **多个 order book** 需聚合 (SIP vs direct feed) |

### 4.3 交易代码 (Ticker symbols)

识别工具的唯一编码（股票、衍生品到期/行权等）。

| HFT 视角 |
|----------|
| Symbology mapping（ISIN/CUSIP/exchange symbol）是 **数据管道** 基础 |

---
