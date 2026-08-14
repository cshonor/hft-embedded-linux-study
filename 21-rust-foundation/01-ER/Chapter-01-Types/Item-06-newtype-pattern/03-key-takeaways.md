# Item 6 · 重点结论

← [Item 6 目录](./README.md)

1. **强业务语义、易混单位/ID** → 用 Newtype，别只靠 `type` 别名。
2. **FFI / 布局等价** → `#[repr(transparent)]`，与内部类型内存布局一致，零开销。
3. **为第三方类型 impl 标准库/serde 等 trait** → 包一层本地 Newtype 是标准做法。
4. **配合 Item 1**：多 `bool` 参数歧义也可优先 **enum**，Newtype 适合「同底层、不同语义」。

---
