# 2.3.1 · 孤儿规则 (Orphan Rule)

> 所属：**Traits and Trait Bounds · 相干性** · [← 07 hub](./07-coherence-orphan-rule.md)

← [06 泛型 Trait](./06-generic-traits.md) · 下一节 [07.2 Coverage 与 Blanket](./07-2-coverage-blanket.md)

---

Rust 强制限制 `impl Trait for Type` 的编写位置，保证全局 `(类型, trait)` 组合**至多一份** impl。

---

## 一、核心目标

杜绝多个 crate 为同一组（外部类型 + 外部 Trait）各自实现，避免：

1. **编译期歧义** — 编译器不知选哪份 impl  
2. **运行时不可预测** — 若允许链接期随机选 impl，行为无法保证  

这套规则统称 **coherence（相干性）**；其中对 impl 落点最核心的约束叫 **孤儿规则（Orphan Rule）**。

---

## 二、孤儿规则

### 规则原文

实现 `trait for 类型` 时，**Trait 和类型二者至少有一个是当前 crate 内定义**。

### 两条关键约束

1. **禁止场景（双外部）**  
   当前 crate 既没定义该 Type、也没定义该 Trait，不能写 `impl ForeignTrait for ForeignType`。  
   例：标准库 `Vec`（外部类型）+ `serde::Serialize`（外部 Trait），你的项目里不能直接 `impl Serialize for Vec<u8>`。

2. **设计初衷**  
   若放开限制，库 A、库 B 都给 `Vec` 实现 `Serialize`，项目同时依赖 A、B 时编译器无法区分该选用哪一份 impl → **E0117** 或一致性冲突。

### 合法 / 非法对照

```rust
// ❌ 非法：Vec、外部 Trait 均来自外部 crate
// impl std::fmt::Display for Vec<u8> {}

// ✅ 合法 1：Trait 本地，类型外部
trait MySerialize {
    fn tag(&self) -> &'static str;
}
impl MySerialize for Vec<u8> {
    fn tag(&self) -> &'static str { "vec-u8" }
}

// ✅ 合法 2：类型本地，Trait 外部
struct MyVec(Vec<u8>);
impl std::fmt::Display for MyVec {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        write!(f, "MyVec({:?})", self.0)
    }
}
```

→ 可运行示例：[orphan-rule-demo](./orphan-rule-demo/)

→ 下一节：[07.2 Coverage 与 Blanket](./07-2-coverage-blanket.md)
