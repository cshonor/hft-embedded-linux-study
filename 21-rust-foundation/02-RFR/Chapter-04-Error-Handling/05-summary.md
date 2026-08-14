# 3. Summary（小结）

> [← 章索引](./README.md)

```text
Representing Errors（枚举 · 不透明 · 特殊情形）
        ↓
Propagating Errors（From + ? · Try）
```

## 三句话

1. 错误是 **API** — 在可分支 `enum` 与可组合 opaque 之间按调用者需求选。  
2. **`Error` / Display / Send / Sync / `'static`** 提升可组合性。  
3. 注意 **`Option` vs `Result`**、`!`、线程 **`Any`** 等特殊形态。

## 下一章

→ [第 5 章 Project Structure](../Chapter-05-Project-Structure/README.md)

## 索引

[01](./01-enumeration.md) · [02](./02-opaque-errors.md) · [03](./03-special-error-cases.md) · [04](./04-propagating-errors.md)
