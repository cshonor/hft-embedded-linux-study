# Item 4 · 重点结论

← [Item 4 目录](./README.md)

| 场景 | 建议 |
|------|------|
| **任何自定义错误** | 实现 `Error`（+ `Display` / `Debug`），别裸 `String` |
| **配合 `?`** | 为子错误实现 `From`（如 `From<io::Error>`），`?` 自动 `.into()`，少写 `map_err` |
| **库 (library)** | 对外暴露**具体、详尽**的 `enum`；用 **`thiserror`** 生成样板；**不要**把宏依赖 leak 到公共 API |
| **应用 (binary)** | 汇总多库异构错误 → **`anyhow::Error`** / `Box<dyn Error>`；堆栈、上下文等 |

---
