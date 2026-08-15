# 1.1.2 · `into_` 系列 — 所有权转移

> 所属：**Unsurprising / 命名惯例** · [← 01 命名 hub](./01-naming-practices.md)

← [01-1 `as_`](./01-1-as-series.md) · 下一节 [01-3 `get_`](./01-3-get-series.md)

Demo → `cargo run --manifest-path naming-series-demo/Cargo.toml into`

→ 包装类型：[04 Wrapper Types](./04-wrapper-types.md)

---

## 核心逻辑

| 点 | 说明 |
|----|------|
| **所有权** | **消耗** `self`，原变量**失效**（move） |
| **返回值** | 内部 owned 值、owned 迭代器、转换后类型 |
| **口诀** | `into` = 交出自己，用完本体就没了 |

```text
Vec<T> ──into_iter()──► impl Iterator<Item = T>   （v 不可再用）
String ──into_bytes()──► Vec<u8>
Box<T> ──into_inner()──► T
```

| 常见 API | 作用 |
|----------|------|
| **`into_iter()`** | 消耗容器 → **owned** 元素迭代器 |
| **`into_inner()`** | 消耗**包装器** → 内部 `T`（见 [01-2-1](./01-2-1-into-inner.md)） |
| **`into_string()` / `into_vec()`** | 类型转换，消耗原值 |

---

## 可运行示例

### `Vec::into_iter` — 容器被 move

```rust
let v = vec![1, 2, 3];
let mut v_iter = v.into_iter();
// println!("{:?}", v); // ❌ 编译错误：v 已被 into_iter move
assert_eq!(v_iter.next(), Some(1));
assert_eq!(v_iter.next(), Some(2));
assert_eq!(v_iter.next(), Some(3));
assert_eq!(v_iter.next(), None);
```

### `String::into_bytes`

```rust
let s = String::from("hi");
let bytes = s.into_bytes();
assert_eq!(bytes, b"hi");
// s 已失效
```

→ demo：`naming-series-demo/src/into_series.rs`

---

## 深入：`into_inner` vs 误传的 `into_item`

**`into_inner`** — 拆包装拿内部所有权（`Mutex::into_inner`、`RefCell::into_inner`、自定义 newtype）。  
⚠️ **`MutexGuard` 没有 `into_inner`**；std **也没有** `Peekable::into_item`，peek 后用 **`next()`**。

→ 详例 + 勘误：[01-2-1 `into_inner`](./01-2-1-into-inner.md) · demo：`cargo run … into-inner`

---

## 与迭代器三巨头

| 方法 | 元素 | 容器 |
|------|------|------|
| `iter()` | `&T` | 保留 |
| `iter_mut()` | `&mut T` | 保留 |
| **`into_iter()`** | **`T`** | **消耗** |

`for x in v` 等价 `v.into_iter()` — 见 [01 命名 §六](./01-naming-practices.md#六迭代器三巨头iter--iter_mut--into_iter)。

---

## 速记

**口诀**：交出自己 · 原变量失效 · `into_iter` · `into_inner`（包装器上）· `into_bytes` — std 无 `into_item`，peek 后用 `next()`

→ 下一节：[01-3 `get_`](./01-3-get-series.md)
