## 7. 本章总结

| 要点 | 含义 |
|------|------|
| **三类成本** | 显性 · 隐性 · 错失机会 |
| **基准法** | Effective · Realized · IS · VWAP — **各有意涵** |
| **分解** | Effective = Realized + **输给知情者** |
| **偏差** | 拆单、风格、知情、**gaming** |
| **冰山** | 择时 + 机会成本 **> 佣金** |
| **计量** | Roll 反转 · Glosten-Harris 订单流 |

> **HFT 读者 takeaway：** 做市日报 **realized spread @ 1s/5s/60s**；buy-side 用 **IS vs arrival price**。若只优化 **effective spread** 会 **gaming**（Ch 7 代理问题）。`orderbook.go` 加 **成交日志 + 事后 mid** 即可算 **markout** — M3 自然延伸。

---
