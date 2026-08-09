## 本章总结

```
市价单：付 spread → 换 immediacy     → 执行价格风险 + impact
限价单：赚 spread / 更好价 → 承担不成交 → 免费期权让渡
止损单：顺 momentum → 加剧波动
MIT：   逆 momentum → 稳定市场
附加属性：IOC/FOK/冰山 → 控制暴露与成交形态
```

> **HFT 读者 takeaway：** 第四章是 **订单 API 的业务语义**——写 Rust 发单模块时，每个 flag (limit/market/IOC/post-only/stop) 都对应本章的一类 economic trade-off。下一章 LOB 讲这些挂单 **如何在簿中排队与撮合**。

---
