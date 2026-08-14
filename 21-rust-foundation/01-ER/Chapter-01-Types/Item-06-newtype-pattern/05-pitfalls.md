# Item 6 · 易错细节

← [Item 6 目录](./README.md)

### 访问内部值

- 字段通过 **`.0`**（或具名字段）访问，比裸类型多一层。

### Trait 不会自动继承

Newtype **不**自动拥有内部类型的 trait 实现：

| 恢复方式 | 适用 |
|----------|------|
| `#[derive(Clone, Debug, Eq, …)]` | 可 derive 的 trait |
| 手写转发 `self.0.fmt(f)` | `Display` 等复杂 trait |
| `Deref` / `derive_more` | 减少 `.0` 与转发样板（见 §6） |

---
