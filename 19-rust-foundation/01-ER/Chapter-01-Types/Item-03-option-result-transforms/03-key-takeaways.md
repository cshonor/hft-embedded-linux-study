# Item 3 · 重点结论

← [Item 3 目录](./README.md)

1. **少写无意义的 `match`**——只取值或给默认值时用 `if let` / `unwrap_or` / `map` 等，意图更清晰。
2. **库 API：有诊断价值就用 `Result`，别偷换成 `Option`**——「文件不存在」vs「权限拒绝」应留给调用方。
3. **链式转换零成本**——多为 `#[inline]`，优化后与手写 `match` 机器码相当。
4. **在 `&Option<T>` 上操作先 `.as_ref()`**——得到 `Option<&T>`，避免从共享引用里 move 非 `Copy` 值。

---
