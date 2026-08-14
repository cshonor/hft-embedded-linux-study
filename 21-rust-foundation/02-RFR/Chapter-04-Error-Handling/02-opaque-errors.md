# 1.2 Opaque Errors（不透明错误）

> 所属：**Representing Errors** · [← 章索引](./README.md)

当调用者**通常不需要区分**上百种内部失败细节时，枚举会拖垮 API 表面积。

## 常见形态

```rust
Box<dyn std::error::Error + Send + Sync + 'static>
```

或生态 crate：**`anyhow::Error`**。

## 优点

- **组合性**：内部多种 `Result` 经 `?` + `From` 统一到单一对外类型。
- **表面积小**：对外「失败即可」，细节靠日志 / `downcast`。

## `'static` 与 downcast

擦除后具体类型不可见；`'static` 等约束使调用方仍可用 **`Error::downcast_ref`** 在需要时取回具体错误。

## `anyhow::chain()` 与 `source()` 链

`anyhow::Error` 包装多层 **`context`** / 底层 **`Error`** 后，可用 **`chain()`** 遍历整条因果链（等同反复 `source()`，打印 `{err:?}` 也常带多层）。详见 [01 错误链](./01-error-source-chain.md) · [Item 04 demo](../../01-ER/Chapter-01-Types/Item-04-idiomatic-error-types/demo/src/main.rs)。

## 与枚举的取舍

| 倾向 | 选型 |
|------|------|
| 库公开 API、可分支 | `enum` + `thiserror` |
| 应用 / bin、快速组合 | `anyhow` 或 `Box<dyn Error>` |
