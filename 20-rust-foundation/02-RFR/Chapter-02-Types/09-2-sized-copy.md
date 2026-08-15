# 2.5.2 · `Sized` 与 `Copy`

> 所属：**Traits and Trait Bounds · 标记 Trait** · [← 09 hub](./09-marker-traits.md)

← [09.1 标记 Trait 基础](./09-1-marker-basics.md) · 下一节 [09.3 Send/Sync/Unpin](./09-3-send-sync-unpin.md)

---

## 一、`Sized`

| 项 | 说明 |
|----|------|
| **契约** | 编译期能确定**固定** `size_of` |
| **默认** | 泛型 `T` **隐含** `T: Sized` |
| **自动** | `i32`、`String`、`Vec`、普通 struct 等 |
| **例外** | DST：`[T]`、`str`、`dyn Trait` → **`!Sized`** |
| **语法** | `T: ?Sized` = 允许 Sized **或** DST |

```rust
// T 默认隐含 T: Sized；?Sized 放开限制
fn foo<T: ?Sized>(x: &T) {}

fn takes_slice(s: &str) {}      // str 是 !Sized，通过引用使用
fn takes_dyn(e: &dyn Error) {} // trait 对象同理
```

→ DST：[04.1](./04-1-dst-basics.md) · [04.3 dyn/vtable](./04-3-dyn-vtable.md)

---

## 二、`Copy`

| 项 | 说明 |
|----|------|
| **契约** | 可按**二进制位**完整复制；复制后**原变量仍有效**（无 move） |
| **自动** | 所有字段均为 `Copy` 时 struct/元组自动实现 |
| **与 `Clone`** | 实现 `Copy` **必须**同时 `Clone`；`Clone` 显式、可深拷贝堆 |
| **反例** | `String`、`Vec` 有堆所有权 → **不能** `Copy`，只能 `Clone` |

```rust
let a = 10;
let b = a;        // Copy：a、b 都有效

let s1 = String::from("hi");
let s2 = s1;      // Move：s1 失效
// let s3 = s1.clone(); // Clone：两份堆数据
```

→ [Ch01 · 04.2 Move/Copy](../Chapter-01-Foundations/04-2-move-copy-clone.md)

→ 下一节：[09.3 `Send` / `Sync` / `Unpin`](./09-3-send-sync-unpin.md)
