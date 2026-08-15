# 2.4 Trait Bounds（Trait 限定）

> 所属：**Traits and Trait Bounds** · [← 章索引](./README.md)

← [07 相干性与孤儿规则](./07-coherence-orphan-rule.md) · 下一节 → [09 标记 Trait](./09-marker-traits.md)

---

Trait bound 把「类型必须具备的能力」写进签名 — 编译期校验 + 决定**静态**还是**动态**分发。

```text
<T: Trait> / impl Trait  →  静态单态化
&dyn Trait / Box<dyn T>   →  动态 vtable
for<'a> ...               →  HRTB，回调 API 标配
```

前置 → [05 编译与分发](./05-compilation-dispatch.md) · 存在类型 → [10 impl Trait](./10-existential-types.md)

---

## 子节导航

| § | 主题 | 阅读 |
|---|------|------|
| **08.1** | 三种语法 · 静/动态分发 | [08-1-syntax-static-dynamic.md](./08-1-syntax-static-dynamic.md) |
| **08.2** | 高阶 Trait 限定 HRTB | [08-2-hrtb.md](./08-2-hrtb.md) |
| **08.3** | 示例 · 避坑 · 速记 | [08-3-examples-pitfalls.md](./08-3-examples-pitfalls.md) |
| — | 速记 · 自测 |
| — | demo | [trait-bounds-demo](./trait-bounds-demo/) |

**建议阅读顺序**：`08.1` → `08.2` → `08.3`

---

## 一句话

> **`T`/`impl` 静态；`dyn` 异构；双 `impl` 默认不同类型；回调用 `for<'a>`。**

---

## 延伸阅读

- 分发模型 → [05 hub](./05-compilation-dispatch.md)
- Nomicon HRTB → [03 型变](../../04-Rust-Nomicon/03_Lifetime_Variance/03-lifetimes.md)
