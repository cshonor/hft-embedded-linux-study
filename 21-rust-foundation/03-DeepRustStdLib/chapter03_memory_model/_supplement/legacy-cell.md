# 3.4 `Cell`

> 章索引：[第 3 章](./README.md) · 前：[3.3 UnsafeCell](./3.3-unsafecell.md) · 后：[3.5 RefCell](./3.5-refcell.md)

---

## 一句话

**`Cell<T>`** = **`UnsafeCell` + `Copy` 语义的单线程内部可变**：通过 **`get` / `set` / `replace`** 整体替换值，**不能**借出 `&T` / `&mut T` — 适合小、`Copy`、无需引用的场景。

---

## 与 `RefCell` 的分工（勿合并）

| | **`Cell<T>`** | **`RefCell<T>`** |
|--|---------------|------------------|
| **`T` 约束** | **`T: Copy`**（旧 API；现亦常见 `T` 任意 + `replace`） | 任意 `T` |
| **访问方式** | 按值 `get`/`set`，无引用 | `borrow` / `borrow_mut` → `Ref`/`RefMut` |
| **运行时检查** | 无借用计数 | 有（冲突则 panic） |
| **典型用途** | 计数器、标志位、`Rc` 内计数 | 图结构、缓存、单线程共享可变 |

```rust
use std::cell::Cell;
let c = Cell::new(1);
c.set(c.get() + 1);
// let r = c.borrow(); // ❌ Cell 没有 borrow
```

---

## 标准库中的位置

- `std::cell::Cell` — 单线程；
- 常与 **`Rc<Cell<_>>`** 组合（仍非 `Sync`，不能跨线程）。

→ [RFR 07-3 Cell vs RefCell](../../02-RFR/Chapter-01-Foundations/07-3-cell-vs-refcell.md)

---

## HFT 提示

热路径上 **`Cell` 无借用计数开销**；若需 **`&mut` 大块数据** 或 **`T: !Copy`**，用 **`RefCell`** 或 **`Mutex`**，不要硬塞 `Cell`。

---

## 相关

- [3.5 RefCell](./3.5-refcell.md)
- [3.3 UnsafeCell](./3.3-unsafecell.md)
