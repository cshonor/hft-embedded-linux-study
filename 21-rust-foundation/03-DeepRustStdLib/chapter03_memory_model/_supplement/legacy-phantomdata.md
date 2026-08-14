# 3.8 `PhantomData`

> 章索引：[第 3 章](./README.md) · 前：[3.7 RwLock](./3.7-rwlock.md) · 后：[3.9 泄漏与循环引用](./3.9-leaks-and-cycles.md)

---

## 一句话

**`PhantomData<T>`** 是 **零大小标记**：在类型里 **不持有 `T`**，却告诉编译器 **「逻辑上拥有 / 借用 / 变型关系」** — 影响 **drop 检查、Send/Sync、variance**，标准库与自研容器里极常见。

---

## 典型用途

| 用途 | 例子 |
|------|------|
| **逻辑生命周期** | 裸指针结构体「假装」持有 `&'a T` |
| **drop check** | 未用到的泛型 `T` 仍须按 `T`  drop |
| **变型标记** | `PhantomData<fn(T)>` 使类型对 `T` **不变** |
| **!Send 标记** | `PhantomData<Rc<()>>` 撤销 `Send`（慎用） |

```rust
use std::marker::PhantomData;

struct Iter<'a, T> {
    ptr: *const T,
    _marker: PhantomData<&'a T>,
}
```

---

## 常见误解

- **`PhantomData` 不分配内存**（ZST）。
- **不能单靠它「伪造」安全** — 仍须 unsafe 不变量自证。
- **不自动实现 `Send`/`Sync`** — 见 [RFR Ch11 §06](../../02-RFR/Chapter-11-Foreign-Function-Interfaces/06-safety.md)

→ [Nomicon 03 · PhantomData](../../04-Rust-Nomicon/03_Lifetime_Variance/06-phantom-data.md) · [RFR drop check](../../02-RFR/Chapter-09-Unsafe-Code/09-drop-check.md)

---

## 相关

- [3.1 布局](./3.1-layout-and-alignment.md) — ZST
- [3.10 MaybeUninit](./3.10-maybeuninit.md) — 另一类「未持有有效 T」的表示
