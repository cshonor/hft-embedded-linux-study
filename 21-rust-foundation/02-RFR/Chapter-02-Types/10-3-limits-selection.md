# 3.3 · 限制 · 选型 · 易混

> 所属：**Existential Types** · [← 10 hub](./10-existential-types.md)

← [10.2 impl vs dyn](./10-2-impl-vs-dyn.md) · 下一节 [11 小结](./11-summary.md)

---

## 一、核心限制

### 1. 不能存异构 `impl Trait`

```rust
// ❌ 不能 Vec<impl Display>
// ✅ 要多种类型共存 → dyn Trait
let items: Vec<Box<dyn Display>> = vec![
    Box::new(1),
    Box::new("hi"),
];
```

### 2. 返回位置只能是一种底层类型

```rust
fn bad(b: bool) -> impl Iterator<Item = i32> {
    if b {
        vec![1, 2].into_iter()   // 类型 A
    } else {
        (0..10)                  // 类型 B — ❌ 编译失败
    }
}
```

每个 `return` 路径的**具体类型必须相同**；分支返回不同迭代器 → 用 `enum` 包装或改 `Box<dyn Iterator<Item = i32>>`。

### 3. 参数位置每个 `impl` 是独立泛型

```rust
fn pair(a: impl Display, b: impl Display) {
    // a、b 可以是不同类型（如 i32 和 &str）
}

fn same<T: Display>(a: T, b: T) {
    // 需要同一类型时必须命名 T
}
```

---

## 二、选型一句话

| 需求 | 选型 |
|------|------|
| 隐藏复杂返回类型、零开销 | **`impl Trait`** |
| `Vec` 里混多种实现 | **`dyn Trait` + 间接** |
| 两个参数必须同类型 | **命名泛型 `<T>`** |
| 热路径、可内联 | 优先 **`impl Trait` / 泛型** |

---

## 三、易混区分

| 易混 | 纠正 |
|------|------|
| `impl Trait` = 运行时多态 | **静态分发**；编译器知道具体 `T` |
| `impl Trait` 可放 `Vec` 元素类型 | 不行；集合元素类型须在签名层固定 |
| 两个 `impl Display` = 同类型 | **独立**泛型参数，默认可不同 |
| `async fn` 返回 `dyn Future` | 默认 **`impl Future`**，非 trait 对象 |
| `∃` vs `∀` 可互换 | `impl Trait` 存在量化；`for<'a>` 全称量化 |

---

## 对照阅读

- Book → [10.2.3 impl Trait 全解](../../00-Book/10-generics-traits-lifetimes/10.2.3-impl-Trait全解.md) · [10.2 trait](../../00-Book/10-generics-traits-lifetimes/10.2-trait.md)
- ER → [Item 12 泛型 vs trait 对象](../../01-ER/Chapter-02-Traits/Item-12-generics-vs-trait-objects/README.md)
- RFR → [05 编译与分发](./05-compilation-dispatch.md) · [08.2 HRTB](./08-2-hrtb.md)

→ 下一节：[11 小结](./11-summary.md)

---

## 速记

## 三句话

1. **`impl Trait` = ∃T：存在某类型，对外匿名，编译期定死。**  
2. **返回位藏长类型；参数位 = 匿名 `<T: Trait>`。**  
3. **要异构集合 → `dyn`；要同参同型 → 命名 `T`。**

## 自测

- [ ] `impl Trait` 与 `dyn Trait` 分发机制差在哪？  
- [ ] 为何 `if/else` 不能返回两种不同迭代器（裸 `impl Iterator`）？  
- [ ] `∃` 与 `for<'a>`（`∀`）各对应什么语法？

