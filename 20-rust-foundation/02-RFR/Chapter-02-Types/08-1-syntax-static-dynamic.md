# 2.4.1 · Trait Bound 语法与静/动态分发

> 所属：**Traits and Trait Bounds · Trait 限定** · [← 08 hub](./08-trait-bounds.md)

← [07 相干性与孤儿规则](./07-coherence-orphan-rule.md) · 下一节 [08.2 HRTB](./08-2-hrtb.md)

---

Trait bound 把「类型必须具备的能力」写进函数签名，让编译器在**编译期**完成类型校验，并触发**单态化**（为每种传入类型生成专属机器码）。

---

## 一、Trait Bound 核心作用

| 作用 | 说明 |
|------|------|
| **能力约束** | 只有实现了所需 trait 的类型才能调用 |
| **编译期检查** | 违反 bound → 编译失败，而非运行时才发现 |
| **单态化入口** | `T: Trait` / `impl Trait` 触发静态分发特化 |
| **API 文档化** | 签名即契约：调用方需要哪些行为 |

→ 分发原理 [05.1 静/动态分发](./05-1-static-vs-dynamic.md)

---

## 二、三种基础语法与分发模式

### 1. 泛型约束 `<T: Trait>`

```rust
use std::fmt::Debug;

fn f<T: Debug + Clone>(x: T) {
    let y = x.clone();
    println!("{y:?}");
}
```

| 项 | 说明 |
|----|------|
| 分发 | **静态分发** |
| 原理 | 编译期为每个不同的 `T` 生成独立函数副本（单态化） |
| 多约束 | `+` 连接多个 trait，须**同时**满足 |
| where 等价 | `fn f<T>(x: T) where T: Debug + Clone {}` |

**何时用**：需要**命名** `T`、多参数**同一** `T`、或返回类型也是 `T`。

---

### 2. 参数位置 `impl Trait`

```rust
use std::fmt::Display;

fn g(x: impl Display) {
    println!("{x}");
}
```

| 项 | 说明 |
|----|------|
| 本质 | 匿名泛型参数的语法糖，同样**静态分发** |
| 语义 | **存在类型**：调用方传入任意实现 `Display` 的类型 |
| 限制 1 | 函数体内**不能**把参数类型提取为具名 `T`（除非改签名） |
| 限制 2 | 多个 `impl Trait` 参数默认是**不同**泛型，除非手写 `T` 绑定 |

```rust
// ❌ 两个 impl Display 不一定是同一类型
fn bad(a: impl Display, b: impl Display) {}

// ✅ 强制同类型
fn good<T: Display>(a: T, b: T) {}
```

---

### 3. `&dyn Trait`（Trait 对象）

```rust
use std::error::Error;

fn h(e: &dyn Error) {
    eprintln!("err: {e}");
}
```

| 项 | 说明 |
|----|------|
| 分发 | **动态分发** |
| 原理 | 胖指针 = 数据指针 + **vtable**，运行时查表调用 |
| 开销 | 间接调用 + 通常无法跨调用内联 |
| 适用 | 集合存多种实现：`Vec<Box<dyn Error>>` |

→ DST / 宽指针：[04 hub](./04-dst-wide-pointers.md) · [04.3 dyn](./04-3-dyn-vtable.md) · [05.1 动态分发](./05-1-static-vs-dynamic.md)

---

### 三者核心区别

| 写法 | 分发 | 运行时开销 | 能否混存多种类型 |
|------|------|------------|------------------|
| `<T: Trait>` | 静态 | 无 vtable | 否（每处调用点一种 `T`） |
| `impl Trait` | 静态 | 无 vtable | 否 |
| `&dyn Trait` / `Box<dyn Trait>` | 动态 | vtable 间接调用 | **是** |

**HFT 直觉**：热路径优先 `T: Trait` / `impl Trait`；仅在必须异构集合或减二进制体积时用 `dyn`。

→ 下一节：[08.2 HRTB](./08-2-hrtb.md)

---

## 速记

## 子节速记

```text
08.1  <T: Trait> · impl Trait · &dyn Trait 三分法
08.2  for<'a> HRTB · 回调吃任意生命周期 &str
08.3  双 impl 陷阱 · demo · 避坑表
```

## 三句话

1. **`T` / `impl` → 静态；`dyn` → vtable。**  
2. **两参数要同类型 → `<T>`，不要双 `impl`。**  
3. **`F: for<'a> Fn(&'a str)` — 回调标准写法。**

## 自测

- [ ] 对比 `fn f(x: impl Display)` 与 `fn f(x: &dyn Display)`  
- [ ] 写出 HRTB 版「接收任意 `&str` 闭包」签名  
- [ ] 为何 `Vec<Box<dyn Error>>` 需要 object-safe trait？

