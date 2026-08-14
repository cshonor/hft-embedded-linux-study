# 2.2.2 · 存在类型 · trait object · HFT

> 所属：**Traits and Trait Bounds · 泛型 Trait** · [← 06 hub](./06-generic-traits.md)

← [06.1 关联类型 vs 泛型参数](./06-1-associated-vs-generic.md) · 下一节 [07 相干性与孤儿规则](./07-coherence-orphan-rule.md)

前置 → [04.3 dyn 与 vtable](./04-3-dyn-vtable.md) · [05.1 静/动态分发](./05-1-static-vs-dynamic.md)

---

## 一、与存在类型 / trait object

| 写法 | 分发 | 关联类型 |
|------|------|----------|
| **`impl Iterator<Item = u32>`** | **静态** — 隐藏具体迭代器类型 | 在 bound 里 **指定** `Item` |
| **`dyn Iterator<Item = u32>`** | **动态** — vtable + 宽指针 | **必须**写清 `Item`（及其他 GAT 若适用） |
| **`impl Trait` 返回** | 静态，HFT 热路径友好 | 见 [10](./10-existential-types.md) |

```rust
fn count_u32(it: impl Iterator<Item = u32>) -> usize { it.count() }

fn count_dyn(it: &mut dyn Iterator<Item = u32>) -> usize { it.count() } // 动态
```

带 **未指定关联类型的 `dyn Iterator`** 通常 ❌ — 编译器不知 `Item` 大小与 vtable 布局。

→ [04 DST / 宽指针](./04-dst-wide-pointers.md) · [05 静态 vs 动态](./05-compilation-dispatch.md)

---

## 二、HFT 实战要点

### 1. 固有属性 → 关联类型

```rust
trait TickSource {
    type Tick; // 固定一种 Tick 结构
    fn next(&mut self) -> Option<Self::Tick>;
}
```

意图清晰；少一层 trait 泛型参数；利于 monomorph inline。

### 2. 跨类型交互 → 泛型 trait

```rust
trait FromWire<T> {
    fn decode(raw: T) -> Self;
}
// 或标准库 From<T> / TryFrom<T>
```

不同 wire 格式、不同 `T` — 才用泛型 trait 参数。

### 3. 警惕泛型 trait 导致代码膨胀

每个 `(Self, Trait, Rhs)` 组合可能 **单独 monomorph** → 编译时间与二进制 ↑。HFT 里 trait 设计宜 **少而精**。

### 4. 核心路径：`impl Trait`，慎用 `dyn`

- 返回迭代器 / 策略组件：`-> impl Iterator<Item = Tick>`  
- 插件边界才 `Box<dyn ...>`，且写全 `Item = ...`

### 5. GAT（了解）

Rust 的 **Generic Associated Types** 把关联类型也参数化 — 高级场景（如 lending iterator）。日常 HFT 先掌握 **普通关联类型 vs 泛型 trait** 即可。

---

## 三、易错点

| 易错 | 纠正 |
|------|------|
| 凡事 `Trait<Rhs>` | 固有 `Item` 用 **关联类型** |
| `dyn Iterator` 不写 `Item` | 须 **`dyn Iterator<Item = T>`** |
| 关联类型 = 性能差 | 与泛型 trait **都能静态分发**；差异在 **能 impl 几份** |
| `impl Iterator` 和 `dyn Iterator` 一样快 | **`impl` 静态**，`dyn` 有 vtable |

---

## 对照阅读

- Book → [10.2 trait](../../00-Book/10-generics-traits-lifetimes/10.2-trait.md)
- ER → [Item 13 默认实现](../../01-ER/Chapter-02-Traits/Item-13-default-implementations/README.md) · [Item 12 分发](../../01-ER/Chapter-02-Traits/Item-12-generics-vs-trait-objects/README.md)

→ 下一节：[07 相干性与孤儿规则](./07-coherence-orphan-rule.md)
