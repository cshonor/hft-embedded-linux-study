# 3.3 `UnsafeCell`

> 章索引：[第 3 章](./README.md) · 前：[3.2 生命周期](./3.2-lifetimes-and-borrow-check.md) · 后：[3.4 Cell](./3.4-cell.md)

---

## 一句话

**`UnsafeCell<T>`** 是 Rust 内部可变性的 **唯一合法根**：在持有 **`&UnsafeCell<T>`**（共享不可变外观）时，仍可通过 **`get()`** 得到 **`*mut T`** 做可变访问 — **`Cell` / `RefCell` / `Mutex` 底层都依赖它**。

---

## 为何需要它

普通 **`&T`** 向 LLVM 承诺「通过该引用不会改 `T`」→ 允许激进优化。  
要在 **共享引用外表** 下修改，必须：

1. 用 **`UnsafeCell`** 包一层，**opt-out** 该假设；
2. 外层类型 **自己** 用运行时或锁保证安全（`RefCell` 计数、`Mutex` 互斥）。

---

## API 要点

```rust
use std::cell::UnsafeCell;

let u = UnsafeCell::new(0);
let r1 = &u;
let r2 = &u; // 多个 &UnsafeCell 允许
let p = r1.get(); // *mut i32 — 之后 unsafe 或封装内使用
```

- **不是 `Sync`**（默认）— 跨线程共享 raw 可变需 `Mutex` 等。
- 几乎不直接在业务代码出现 — **读 `RefCell` / `Mutex` 源码时认它**。

→ [RFR 07-2 UnsafeCell](../../02-RFR/Chapter-01-Foundations/07-2-unsafecell-and-containers.md) · [Nomicon 01 five powers](../../04-Rust-Nomicon/01_Safe_Unsafe/03-five-powers.md)

---

## 相关

- [3.4 Cell](./3.4-cell.md) · [3.5 RefCell](./3.5-refcell.md)
