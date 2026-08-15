# 1.3 Ergonomic Trait Implementations（人体工程学 impl）

> 所属：**Unsurprising** · [← 章索引](./README.md)

← [02 通用 Trait](./02-common-traits-for-types.md) · 下一节 [04 包装类型](./04-wrapper-types.md)

**目标**：消除调用分叉 — 让 `&T`、`Box<T>` 等也能直接调用**你定义的 trait** 方法。

---

## 核心：Blanket impl（两种形态）

| 形态 | 写法 | 作用 |
|------|------|------|
| **A 条件补能力** | `impl<T: Other> MyTrait for T` | 满足条件的 **T** 自动获得 trait |
| **B 包装转发** | `impl<T: MyTrait> MyTrait for &T` | **引用/指针**也能调，消除 `*` 分叉 |

§03 重点在 **形态 B**；形态 A 绑 `Debug` 等过宽 trait 易冲突。

**两套并行**：`impl MyTrait for Foo` 写业务逻辑；blanket 是 trait 侧的**可选补丁**，Foo 的 impl 里不用提。

```rust
trait MyTrait { fn work(&self); }

impl<T: MyTrait + ?Sized> MyTrait for &T {
    fn work(&self) { (*self).work() }
}
```

---

## 约束（两线）

| 约束 | 要点 |
|------|------|
| **孤儿 / 相干** | 仅**本 crate 自定义 trait**；不能给 `Debug` 等外部 trait 写 blanket |
| **过宽 blanket** | `impl<T: Debug> MyTrait for T` 易与未来 impl 冲突 → 收窄 bound 或 [sealed trait](./12-trait-implementations.md) |

---

## 速记

**两种形态**：A 条件 impl on `T` · B 包装转发 `&T`（本节重点）  
**核心**：blanket → `&T` / `&mut T` / Box/Arc · 仅**自定义 trait**  
**禁**：全域 `impl<T: Debug> …` · 外部 trait 的 blanket

→ 详例：[03-1](./03-1-ergonomic-blanket-full-guide.md) · [`?Sized`](./03-2-question-sized.md) · [demo](./blanket-trait-demo/) · ER [Item 13](../../01-ER/Chapter-02-Traits/Item-13-default-implementations/README.md)
