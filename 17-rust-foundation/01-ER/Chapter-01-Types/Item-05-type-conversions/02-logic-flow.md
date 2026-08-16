# Item 5 · 逻辑脉络

← [Item 5 目录](./README.md)

### `From` / `Into` 的非对称 blanket impl

- 逻辑上对称，但为避免循环 impl 冲突，标准库约定：
  - 你实现 **`From<T> for U`** → 自动获得 **`Into<U> for T`**
  - **不要**再手写 `Into`（除非特殊场景）

### 自反 impl 与 API 人体工学

```rust
impl<T> From<T> for T { ... }  // 标准库已有
```

- 泛型边界写 **`T: Into<MyType>`** 时：
  - 可传入能转成 `MyType` 的类型；
  - 也可**直接**传入 `MyType`（自反 `From`）。

---
