## 2.4.4 舍入 (Rounding)

> **Ch2 §2.4** · [章导读](../README.md) · 上节 [§2.4.3 数字示例](./section-2.4.3-浮点数字示例.md) · 下节 [§2.4.5 浮点运算](./section-2.4.5-浮点运算.md)

---

- 默认 **向偶数舍入 (round to nearest, ties to even)** — 减少长期 bias
- 历史坑：x87 寄存器 **80 bit** 扩展精度 → 同表达式不同优化级别结果可能不同
- 缓解：统一 SSE/AVX、`-ffloat-store`（慢）、或 **不用 float 做账本**

**HFT 铁律（常见）：**

| 场景 | 建议 |
|------|------|
| 价格、PnL、风控阈值 | **整数 tick** 或 decimal 库 / `rust_decimal` |
| 指标、统计 | double 可接受，知悉误差 |
| 相等比较 | **禁止** `a == b`；用 epsilon 或整数 tick 差 |

---

### 口述巩固 · 自测

1. HFT 里为什么价格常用整数 tick 而不是 `double`？

---

← [本章导读](../README.md) · [§2.4.3 ←](./section-2.4.3-浮点数字示例.md) · [§2.4.5 →](./section-2.4.5-浮点运算.md)
