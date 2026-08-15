# 2.2 Generic Traits（泛型 Trait）

> 所属：**Traits and Trait Bounds** · [← 章索引](./README.md)

← [05 编译与分发](./05-compilation-dispatch.md) · 下一节 → [07 相干性与孤儿规则](./07-coherence-orphan-rule.md)

---

**关联类型** = 一对一固有属性（`Iterator::Item`）；**泛型 trait 参数** = 多对多跨类型交互（`PartialEq<Rhs>`、`From<T>`）。

```text
Item/Error/Output  →  关联类型
能和哪些类型比/转  →  泛型 trait 参数
dyn Trait          →  须写清关联类型，如 Item = u32
```

前置 → [05 编译与分发](./05-compilation-dispatch.md)（[05.1](./05-1-static-vs-dynamic.md)）· [04.3 dyn](./04-3-dyn-vtable.md)

---

## 子节导航

| § | 主题 | 阅读 |
|---|------|------|
| **06.1** | 关联类型 vs 泛型参数 · 怎么选 | [06-1-associated-vs-generic.md](./06-1-associated-vs-generic.md) |
| **06.2** | `impl`/`dyn` · HFT · GAT | [06-2-existential-hft.md](./06-2-existential-hft.md) |
| — | 速记 · 自测 |

**建议阅读顺序**：`06.1` → `06.2`

---

## 一句话

> **固有属性用关联类型；多 impl 用泛型参数；`dyn` 必须指定 `Item` 等关联类型。**
