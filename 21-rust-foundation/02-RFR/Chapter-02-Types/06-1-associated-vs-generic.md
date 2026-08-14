# 2.2.1 · 关联类型 vs 泛型 Trait 参数

> 所属：**Traits and Trait Bounds · 泛型 Trait** · [← 06 hub](./06-generic-traits.md)

← [05 编译与分发](./05-compilation-dispatch.md) · 下一节 [06.2 存在类型与 HFT](./06-2-existential-hft.md)

---

Trait 里描述「类型关系」有两种核心机制 — 适用场景和设计直觉完全不同。

---

## 一、对照总表

| 机制 | 同一 `(Type, Trait)` 能有几份「选择」 | 典型 |
|------|--------------------------------------|------|
| **关联类型 (Associated Type)** | 通常 **只有一种** | `Iterator::Item` |
| **泛型 trait 参数** | 可对 **不同类型多次 impl** | `PartialEq<Rhs>`、`From<T>` |

---

## 二、关联类型：「一对一」

**问的是**：*这个类型实现该 trait 时，**对应的**类型是什么？* — 一种 impl 里**唯一确定**。

```rust
trait Iterator {
    type Item;
    fn next(&mut self) -> Option<Self::Item>;
}

impl Iterator for std::vec::IntoIter<i32> {
    type Item = i32; // 对这份 impl，Item 只能是 i32
    fn next(&mut self) -> Option<Self::Item> { /* ... */ }
}
```

**与智能指针对照**：`Deref::Target` 同样是关联类型 — Box 解引用后「只能是内部的 T」→ Book [15.2.3 Deref::Target](../../00-Book/15-smart-pointers/15.2.3-引用智能指针与Target关联类型.md)

| 特点 | |
|------|---|
| 每个 `(Self, Trait)` **一份**关联类型绑定 | 签名简洁：`Iterator` 而非 `Iterator<Item>` |
| 适合描述**固有属性** | 元素类型、Error 类型、Output 等 |
| 与 `dyn` | 作 trait object 时须 **指定关联类型** → [06.2](./06-2-existential-hft.md) |

**HFT 直觉**：订单簿 iterator 的 `Item = OrderId` — 一种结构一种元素，用关联类型表达最清楚。

---

## 三、泛型 trait 参数：「多对多」

**问的是**：*这个 trait 可以和**哪些其它类型**交互？* — 同一 `Self` 可 **多次 impl**。

```rust
trait PartialEq<Rhs = Self> {
    fn eq(&self, other: &Rhs) -> bool;
}

impl PartialEq<u32> for u32 { /* u32 == u32 */ }
impl PartialEq<u64> for u32 { /* u32 == u64，若你实现 */ }
```

```rust
trait From<T> {
    fn from(value: T) -> Self;
}
// 同一 Target 可对多个 T：From<u32>、From<String> …
```

| 特点 | |
|------|---|
| **灵活** — 多种 RHS / 源类型 | 签名更复杂：`PartialEq<Rhs>` |
| 适合 **跨类型** 比较、转换、运算 | `Add<Rhs>`、`TryFrom<T>` 等 |
| monomorph | 每种 `(Self, Trait, Rhs)` 组合可生成一份代码 → 注意**膨胀** |

---

## 四、怎么选（设计直觉）

| 问题 | 倾向 |
|------|------|
| 「这个类型的 **Item/Error/Output** 是什么？」 | **关联类型** |
| 「能和 **哪些其它类型** 比/转/算？」 | **泛型参数** |
| 希望 trait 名简短、impl 块内唯一 | **关联类型** |
| 需要同一 type 上多份不同 RHS 行为 | **泛型参数** |

**默认倾向**：能写成关联类型就写关联类型；只有确实需要 **多 impl** 再用泛型 trait。

→ 下一节：[06.2 存在类型 · trait object · HFT](./06-2-existential-hft.md)

---

## 速记

## 对照

| | 关联类型 | 泛型 trait 参数 |
|---|----------|-----------------|
| 问什么 | 固有 Item/Error | 能和谁比/转 |
| impl 份数 | 通常 1 份 | 可多份 |
| 典型 | `Iterator::Item` | `PartialEq<Rhs>` |

## 三句话

1. **能关联类型就关联类型。**  
2. **`dyn Iterator` → 必须 `Item = T`。**  
3. **`impl` 静态，`dyn` vtable。**

