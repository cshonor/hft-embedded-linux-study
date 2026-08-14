# 1.4 Wrapper Types（包装类型）

> 所属：**Unsurprising** · [← 章索引](./README.md)

← [03 人体工学 Trait 实现](./03-ergonomic-trait-implementations.md) · 下一节 [05 泛型参数](./05-generic-arguments.md)

用 **newtype** 或薄包装表达**单位、权限、阶段**，而不改变运行时表示（或仅多一层 indirection）。

→ **Newtype 完整详解**（四大用途 · 孤儿规则 · vs `type` 别名）→ [Ch02 §07.3](../Chapter-02-Types/07-3-newtype-practice.md) · [ER Item 06](../../01-ER/Chapter-01-Types/Item-06-newtype-pattern/README.md)

## Newtype（速览）

```rust
struct Meters(f64);
struct UserId(u64);
```

- 防止 `Meters + Seconds` 这类逻辑错误。
- 可为外部类型 impl 本地 trait（孤儿规则）。

## 智能指针式包装

| 类型 | 额外语义 |
|------|----------|
| `Box<T>` | 堆分配、唯一拥有 |
| `Arc<T>` | 共享拥有 |
| 自定义 `Guard` | RAII、权限令牌 |

## `Deref` 包装

- 为 API 一致性实现 `Deref` / `DerefMut`—— **勿过度** Deref 到整个内部容器，否则校验被绕过 → Book [15.2](../../00-Book/15-smart-pointers/15.2-通过Deref将智能指针当作引用.md) · [15.2.2 速记/`*`/unsafe](../../00-Book/15-smart-pointers/15.2.2-Deref解引用与unsafe速记.md) · ER [over_deref demo](../../01-ER/Chapter-01-Types/Item-05-type-conversions/demo/src/over_deref.rs)

## 与命名配合

- `into_inner()`、`as_ref()` 等应与本章 [01 命名](./01-naming-practices.md) 一致。
