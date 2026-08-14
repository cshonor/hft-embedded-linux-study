# Item 6 · 逻辑脉络

← [Item 6 目录](./README.md)

```text
裸 f64 / bool（语义易混）
  → type 别名（仅文档，无安全）
  → Newtype（编译期防混）
  → 必要时 From/TryFrom 做合法转换
  → 外部类型 + 外部 trait → Newtype 突破孤儿规则
```

---
