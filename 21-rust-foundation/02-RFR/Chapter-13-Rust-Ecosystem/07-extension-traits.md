# 2.3 Extension Traits（扩展 Trait）

> 所属：**Patterns in the Wild** · [← 章索引](./README.md)

绕过孤儿限制的安全做法：定义 **`MyExt`**，对 **`T: Iterator`** 等做 **blanket `impl`**。

```rust
trait MyExt: Iterator { ... }
impl<I: Iterator> MyExt for I { ... }
```

示例：**`itertools`**、**`tower::ServiceExt`**。

→ [第 3 章 · 人体工程学 impl](../Chapter-03-Designing-Interfaces/03-ergonomic-trait-implementations.md) · [第 2 章 · 孤儿规则](../Chapter-02-Types/07-coherence-orphan-rule.md)
