# Item 26 · 逻辑脉络

← [Item 26 目录](./README.md)

```text
cfg（编译器底层）
         ↓
features（Cargo 包级开关）
         ↓
全局 unification → 特性必须「纯加法」
         ↓
互斥 feature / 字段门控 → 下游无法控、必炸
         ↓
克制数量 + CI 测组合（cargo-hack）
```

---
