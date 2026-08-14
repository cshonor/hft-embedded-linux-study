# 1.1.3 · `get_` 系列 — 安全访问

> 所属：**Unsurprising / 命名惯例** · [← 01 命名 hub](./01-naming-practices.md)

← [01-2 `into_`](./01-2-into-series.md) · 下一节 [01-4 `try_`](./01-4-try-series.md)

Demo → `cargo run --manifest-path naming-series-demo/Cargo.toml get`

---

## 核心逻辑

| 点 | 说明 |
|----|------|
| **所有权** | **借用**，不消耗容器 |
| **失败时** | 返回 **`None`**，**不 panic** |
| **返回值** | `Option<&T>` / `Option<&mut T>` |
| **口诀** | 安全索引，越界不崩 |

```text
v.get(i)     ──越界──► None
v[i]         ──越界──► panic!
```

| 对比 | 越界 | 返回 |
|------|------|------|
| **`v[i]`** | **panic** | `&T` |
| **`v.get(i)`** | `None` | `Option<&T>` |
| **`v.get_mut(i)`** | `None` | `Option<&mut T>` |

适用：`Vec`、切片、`[T; N]`、`HashMap::get` 等。

---

## 可运行示例

### 越界 → `None`

```rust
let v = vec![1, 2, 3];
assert_eq!(v.get(1), Some(&2));
assert_eq!(v.get(5), None); // 越界 → None，不崩溃
// let bad = v[5];          // ❌ panic: index out of bounds
```

### `get_mut` — 安全修改

```rust
let mut v = vec![10, 20, 30];
if let Some(x) = v.get_mut(0) {
    *x += 1;
}
assert_eq!(v, vec![11, 20, 30]);
```

→ demo：`naming-series-demo/src/get_series.rs`

---

## 何时选 `get` vs 下标

| 场景 | 建议 |
|------|------|
| 索引来自用户 / 外部输入 | **`get`** |
| 逻辑上保证合法（刚 `push` 的 `len-1`） | 下标也可 |
| 需要 `?` / `match` 分支处理缺失 | **`get`** |

---

## 速记

**口诀**：`Option` 安全访问 · 越界 `None` 不 panic · `v[5]` panic · `v.get(5)` → `None`

→ 下一节：[01-4 `try_`](./01-4-try-series.md)
