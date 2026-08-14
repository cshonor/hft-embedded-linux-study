# Item 8 · 易错细节

← [Item 8 目录](./README.md)

### `RefCell` 运行时 panic

| 违规 | 结果 |
|------|------|
| 已有 `borrow`，再 `borrow_mut` | **panic**（编译不拦） |
| 多个 `borrow_mut` | **panic** |

→ 用 **`try_borrow` / `try_borrow_mut`** 若需优雅失败。

### `Rc` 强引用环

```text
Parent --Rc--> Child
Child  --Rc--> Parent   // strong count 永不为 0 → 泄漏
```

→ 回指用 **`Weak`**，需要时再 `upgrade()`。

### `Deref` vs `AsRef`

- **`Deref`**：编译器自动 coercion，**一个** `Target`。
- **`AsRef`**：手动、可多个 impl，API 设计用（Item 5 待补）。

---
