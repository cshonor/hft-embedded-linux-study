# 4.2 Trait Implementations（Trait 实现控制）

> 所属：**Constrained** · [← 章索引](./README.md)

控制**谁能为你的 trait 提供 impl**，防止生态分叉与不变式破坏。

## Sealed trait（密封 trait）

惯用法：私有父 trait + 公开子 trait，使 trait **对外可用、对外不可 impl**。

```rust
mod sealed {
    pub trait Sealed {}
}
pub trait MyTrait: sealed::Sealed { ... }
impl sealed::Sealed for MyType {}
```

## 用途

- 限定 impl 集合，便于内部优化或改变语义。
- 与 blanket impl 配合时避免「第三方类型也获得 impl」。

## 与孤儿规则

- Sealed 不替代孤儿规则；二者互补 → [07 相干性与孤儿规则](../Chapter-02-Types/07-coherence-orphan-rule.md)

## 下游 impl 需求

若用户**必须**扩展行为，提供 **composition**（包装类型、回调 trait）而非开放继承式 impl。
