# 2.4.3 · 示例 · 避坑 · 速记

> 所属：**Traits and Trait Bounds · Trait 限定** · [← 08 hub](./08-trait-bounds.md)

← [08.2 HRTB](./08-2-hrtb.md) · 下一节 [09 标记 Trait](./09-marker-traits.md)

---

## 一、实操示例

→ 可运行：[trait-bounds-demo](./trait-bounds-demo/)

```bash
cd 02-RFR/Chapter-02-Types/trait-bounds-demo
cargo run
```

### 静态分发（`impl Trait`）

```rust
use std::fmt::Display;

fn print(x: impl Display) {
    println!("{x}");
}

fn main() {
    print(123);
    print("hello");
}
```

### 动态分发（`dyn Trait`）

```rust
use std::error::Error;

fn log_err(e: &dyn Error) {
    eprintln!("err: {e}");
}
```

### HRTB 完整示例

```rust
fn takes_any_str_closure<F>(f: F)
where
    F: for<'a> Fn(&'a str),
{
    f("short lifetime");
    let owned = String::from("long string");
    f(&owned);
}

fn main() {
    takes_any_str_closure(|s| println!("got: {s}"));
}
```

去掉 `for<'a>` 后，与「固定外部 `'a`」的窄约束等价的写法往往会**编译失败**。

---

## 二、避坑清单

### 静态 vs 动态

| 坑 | 纠正 |
|----|------|
| 到处用 `Box<dyn Trait>`「更灵活」 | 热路径优先泛型；`dyn` 有 vtable + 难 inline |
| 以为 `impl Trait` 能混类型集合 | 不能；要异构集合用 `dyn` 或枚举 |
| 返回 `impl Trait` 与参数 `impl Trait` 混淆 | 返回位置是**存在类型**出口；见 [10 存在类型](./10-existential-types.md) |

### `impl Trait` 参数

| 坑 | 纠正 |
|----|------|
| `fn f(a: impl T, b: impl T)` 当同一类型 | 编译器视为**两个**匿名泛型 → 改 `fn f<T: T>(a: T, b: T)` |
| 在函数里需要 `T: Default` 等额外 bound | 改用显式 `<T>` + where |

### HRTB

| 坑 | 纠正 |
|----|------|
| `F: Fn(&'a str)` + 函数级 `'a` | 过窄；改 `for<'a> Fn(&'a str)` → [08.2](./08-2-hrtb.md) |
| 与生命周期省略规则打架 | 回调签名优先查是否该 HRTB |
| 以为 HRTB 是运行时概念 | **纯编译期**；影响类型是否可赋值 |

### Trait bound 其它

| 坑 | 纠正 |
|----|------|
| bound 写太多导致编译慢/二进制大 | 拆函数、用 `dyn` 减单态化、或关联类型收敛 |
| `T: 'static` 乱加 | 会拒绝借用的非 `'static` 数据；只在确实需要时加 |

---

## 三、速记

1. **`T: Trait` / `impl Trait` → 静态单态化；`dyn Trait` → vtable 动态。**  
2. **多参数要同一类型 → 显式 `<T>`，不要双 `impl`。**  
3. **回调吃任意 `&str` → `for<'a> Fn(&'a str)`。**

---

## 对照阅读

- Book → [10.1 泛型](../../00-Book/10-generics-traits-lifetimes/10.1-泛型数据类型.md) · [10.2 trait](../../00-Book/10-generics-traits-lifetimes/10.2-trait.md) · [10.2.3 impl Trait](../../00-Book/10-generics-traits-lifetimes/10.2.3-impl-Trait全解.md)
- RFR → [Ch01 · 08 生命周期](../Chapter-01-Foundations/08-lifetimes.md)
- ER → [Item 02 闭包与 trait bound](../../01-ER/Chapter-01-Types/Item-02-express-common-behavior/README.md) · [Item 12 泛型 vs trait 对象](../../01-ER/Chapter-01-Types/Item-12-generics-vs-trait-objects/README.md)

→ 下一节：[09 标记 Trait](./09-marker-traits.md)
