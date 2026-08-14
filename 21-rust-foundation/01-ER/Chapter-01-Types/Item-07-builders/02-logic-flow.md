# Item 7 · 逻辑脉络

← [Item 7 目录](./README.md)

```text
全字段手写初始化（冗长、易随字段变化腐化）
  → Default + ..（字段都要 Default）
  → Builder（构造逻辑与类型解耦，可选字段友好）
  → derive_builder 等宏（少手写样板）
```

---
