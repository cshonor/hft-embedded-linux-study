# 2.3 Borrowed vs. Owned（借用 vs 拥有）

> 所属：**Flexible** · [← 章索引](./README.md)

API 应**诚实**表达是否需要所有权，避免「收借用再内部 clone」的隐藏成本。

## 何时要拥有

- 函数需**跨调用存储**数据、spawn 线程、转移给另一所有者 → 参数类型应要求 **`String` / `Vec<T>` / `T`** 等 owned。
- 不要收 `&str` 再在内部 `.to_string()` 除非文档明确说明会分配。

## 何时借用足够

- 只读扫描、格式化、临时计算 → `&str`、`&[u8]`、`impl AsRef<T>`。

## `Cow<'a, T>`

当**有时借用、有时需拥有**时，用 `Cow` 显式建模：

```rust
fn process(s: Cow<'_, str>) { ... }
```

调用方传 `&str` 零分配；需要拥有时再 `into_owned()`。

## 与生命周期

- 借用 API 必须写清生命周期；见 [RFR 第 1 章](../Chapter-01-Foundations/08-lifetimes.md)。

Book → [4.2 引用与借用](../../00-Book/04-ownership/4.2-引用与借用.md)
