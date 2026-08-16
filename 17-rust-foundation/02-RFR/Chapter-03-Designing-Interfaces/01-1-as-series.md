# 1.1.1 · `as_` 系列 — 只读借用

> 所属：**Unsurprising / 命名惯例** · [← 01 命名 hub](./01-naming-practices.md)

← [01 命名 hub](./01-naming-practices.md) · 下一节 [01-2 `into_`](./01-2-into-series.md)

Demo → [`naming-series-demo/`](./naming-series-demo/) · `cargo run --manifest-path naming-series-demo/Cargo.toml as`

---

## 核心逻辑

| 点 | 说明 |
|----|------|
| **所有权** | **不消耗** `self`，只借出引用视图 |
| **返回值** | `&T` / `&str` / `Option<&T>` 等 |
| **可重复调用** | 同一变量可多次 `as_*`，本体仍有效 |
| **口诀** | `as` 只借、不拿走 |

```text
String ──as_str()──► &str        （s 仍拥有堆数据）
Option<T> ──as_ref()──► Option<&T>
```

| 常见 API | 作用 |
|----------|------|
| **`String::as_str()`** | `&String` → `&str` |
| **`Option::as_ref()`** | `Option<T>` → `Option<&T>` |
| **`Option::as_mut()`** | 可变引用版 |
| **`Result::as_ref()`** | `Result<T,E>` → `Result<&T,&E>` |
| **`as_ptr()`** | 转裸指针 — unsafe 场景 |

---

## 可运行示例

### `String::as_str` — 反复借用

```rust
let s = String::from("hello");
let s1 = s.as_str();
let s2 = s.as_str(); // 原 s 还能用，不会被 move
println!("{}", s);   // hello
assert_eq!(s1, "hello");
assert_eq!(s2, "hello");
```

### `Option::as_ref` — 包装类型只借内部

```rust
let x = Some(10);
let r1 = x.as_ref(); // Some(&10)
let r2 = x.as_ref();
assert!(x.is_some()); // x 仍在
assert_eq!(r1, r2);
```

→ demo：`naming-series-demo/src/as_series.rs`

---

## 与 `into_` 的分界

需要**交出去**、原变量失效 → 用 [`into_`](./01-2-into-series.md)（如 `into_string()`、`into_iter()`）。

---

## 速记

**口诀**：只借，不消耗 · 可反复调用 · 代表 API：`as_str` · `as_ref` · `as_mut`

→ 下一节：[01-2 `into_`](./01-2-into-series.md)
