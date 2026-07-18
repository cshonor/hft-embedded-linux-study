## 2.4.6 C 语言中的浮点数

> **Ch2 §2.4** · [章导读](../README.md) · 上节 [§2.4.5 浮点运算](./section-2.4.5-浮点运算.md) · 下节 [§2.5 小结](./section-2.5-本章小结.md)

---

- 字面量：`3.14f`（float）、`3.14`（double）、`3.14L`（long double）
- **默认 argument promotion：** `float` 传参常 **提升为 double**
- **`printf`：** `%f` / `%a`（hex float，调试精确 bit 时用）
- **`-ffast-math`** — 打破严格 IEEE 换速度；**回测 vs 生产** 是否一致要验证

---

### 口述巩固 · 自测

1. `-ffast-math` 对 HFT 回测有什么风险？

---

← [本章导读](../README.md) · [§2.4.5 ←](./section-2.4.5-浮点运算.md) · [§2.5 →](./section-2.5-本章小结.md)
