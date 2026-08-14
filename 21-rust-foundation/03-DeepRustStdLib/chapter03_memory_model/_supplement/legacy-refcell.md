# 3.5 `RefCell`

> 章索引：[第 3 章](./README.md) · 前：[3.4 Cell](./3.4-cell.md) · 后：[3.6 Mutex](./3.6-mutex.md)

---

## 一句话

**`RefCell<T>`** = **运行时借用检查** + **`UnsafeCell<T>`**：在 **`&RefCell`** 外表下可同时存在多个 **`Ref<T>`** 或一个 **`RefMut<T>`** — **仅单线程**，违反规则 **panic**（非 UB）。

---

## 核心 API

```rust
use std::cell::RefCell;

let r = RefCell::new(vec![1, 2]);
{
    let mut w = r.borrow_mut();
    w.push(3);
} // RefMut drop 后释放借用
let read = r.borrow(); // 多个 Ref 可并存
```

| 方法 | 等价编译期概念 |
|------|----------------|
| `borrow()` | `&T` |
| `borrow_mut()` | `&mut T` |
| `try_borrow*` | 不 panic，返回 `Result` |

---

## 标准库与生态

- **`Rc<RefCell<T>>`** — 单线程共享可变图；
- **`RefCell` 非 `Sync`** — 不能 `Arc<RefCell<_>>` 跨线程（用 **`Mutex`**）；
- `into_inner` 拆包装 → [RFR into_inner](../../02-RFR/Chapter-03-Designing-Interfaces/01-2-1-into-inner.md)

---

## 与 3.9 循环引用

`Rc<RefCell<_>>` 环 → **泄漏**（引用计数永不为 0）— 见 [3.9](./3.9-leaks-and-cycles.md)。

→ [Book 15 RefCell](../../00-Book/15-smart-pointers/) · [RFR 07 内部可变性](../../02-RFR/Chapter-01-Foundations/07-interior-mutability.md)

---

## 相关

- [3.4 Cell](./3.4-cell.md) — 无引用的轻量兄弟
- [3.6 Mutex](./3.6-mutex.md) — 跨线程版「内部可变」
