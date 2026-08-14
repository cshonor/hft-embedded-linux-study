# Item 8 · 逻辑脉络

← [Item 8 目录](./README.md)

### Deref 强制转换

- 智能指针实现 **`Deref` / `DerefMut`** → 调用处可像用 `&T` 一样用 `Box` / `Rc` 等。
- **`Deref::Target` 唯一**（与 `AsRef` 多目标不同）→ 避免 coercion 歧义。

### `AsRef` / `Borrow` / `ToOwned`

| Trait | 作用 |
|-------|------|
| **`AsRef<T>`** | 显式转成多种引用视图（如 `String` → `&str`、`&[u8]`） |
| **`Borrow<T>`** | 容器键查找：`HashMap::get` 同时收 `&K` 和 `K`（有 `Borrow` blanket） |
| **`ToOwned`** | 在 `Borrow` 之上泛化「从借用得到 owned」 |

---
