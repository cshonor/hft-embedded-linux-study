# Item 5 · 重点结论

← [Item 5 目录](./README.md)

1. **实现只写 `From`，边界只写 `Into`**——经典准则。
2. **可能失败 → `TryFrom` / `TryInto`**——用 `Result`，别 `unwrap` 掩盖截断/解析错误。
3. **优先 `From` / `.into()`，慎用 `as`**——`as` 允许**有损**转换；除非 FFI / 底层语义明确需要。
4. **数值「变窄」**——`as` 能静默截断；安全代码用 `TryFrom` 或 Clippy 查 `cast_possible_truncation`。

---
