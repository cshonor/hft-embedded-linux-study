# 2.3 Coherence and the Orphan Rule（相干性与孤儿规则）

> 所属：**Traits and Trait Bounds** · [← 章索引](./README.md)

← [06 泛型 Trait](./06-generic-traits.md) · 下一节 → [08 Trait 限定](./08-trait-bounds.md)

---

保证全局 `(类型, trait)` **至多一份** impl — **coherence**；核心约束是 **孤儿规则**：trait 与类型**至少一方本地**。

```text
双外部 impl     →  ❌
本地 trait      →  ✅ impl MyTrait for ForeignType
本地 type       →  ✅ impl ForeignTrait for MyType
双外部要扩展    →  NewType + Deref
```

---

## 子节导航

| § | 主题 | 阅读 |
|---|------|------|
| **07.1** | 孤儿规则 · 合法/非法 | [07-1-orphan-rule.md](./07-1-orphan-rule.md) |
| **07.2** | Coverage · Blanket impl | [07-2-coverage-blanket.md](./07-2-coverage-blanket.md) |
| **07.3** | Newtype 模式完整详解 | [07-3-newtype-practice.md](./07-3-newtype-practice.md) |
| — | 速记 · 自测 |
| — | demo | [orphan-rule-demo](./orphan-rule-demo/) |

**建议阅读顺序**：`07.1` → `07.2` → `07.3`

---

## 一句话

> **双外部不能 impl；要扩展用 NewType；blanket 别铺太宽。**

---

## 速记

## 三句话

1. **trait 与 type 至少一方本地。**  
2. **Coverage：`impl From<MyLocal> for Foreign`。**  
3. **双外部 → NewType + `Deref`。**

## Newtype 四用途

类型安全 · 绕孤儿 · 校验构造 · 专属方法

→ 详 [07.3](./07-3-newtype-practice.md)

## 自测

- [ ] 为何不能 `impl Display for Vec<u8>`？  
- [ ] blanket `impl<T: Debug> MyTrait for T` 有何风险？  
- [ ] NewType 有运行时开销吗？

