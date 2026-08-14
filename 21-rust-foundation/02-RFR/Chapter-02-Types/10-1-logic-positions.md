# 3.1 · 存在量化与使用位置

> 所属：**Existential Types** · [← 10 hub](./10-existential-types.md)

← [09 标记 Trait](./09-marker-traits.md) · 下一节 [10.2 impl vs dyn](./10-2-impl-vs-dyn.md)

---

Existential Types（存在类型）在 Rust 里就是 **`impl Trait`**。名字来自数理逻辑的**存在量词 ∃**：

> **存在**某一个具体类型 `T`，满足 `T` 实现了指定 trait，但对外**隐藏**这个 `T` 的真实名称。

核心两点：

1. 编译器**全程知道底层真实类型**，编译期大小确定，走**静态分发、单态化**，无运行时开销；
2. 对外只暴露 trait 接口，隐藏内部复杂类型，**稳定 API**、简化签名。

---

## 一、逻辑对应关系

| Rust 语法 | 逻辑量词 | 含义 |
|-----------|----------|------|
| `impl Trait` | **∃** `T`. `T: Trait` | 存在某类型满足约束（编译期唯一确定） |
| HRTB `for<'a>` | **∀** `'a` | 对**所有**生命周期 `'a` 约束都成立 |

→ HRTB：[08.2](./08-2-hrtb.md)

---

## 二、两大使用位置

### 1. 返回值位置（最常用）

```rust
fn iter() -> impl Iterator<Item = i32> {
    vec![1, 2, 3].into_iter().map(|x| x + 1)
}
```

`map` / `filter` / `flat_map` 链式组合后，真实类型名可长达数十行；`impl Iterator` 对外只承诺「返回迭代器」。

`async fn` 的返回值本质也是 **`impl Future`**，自动隐藏编译器生成的匿名 `Future` 状态机：

```rust
async fn fetch() -> u32 { 42 }
// 等价于返回 impl Future<Output = u32>
```

→ Async：[RFR 第 8 章](../Chapter-08-Asynchronous-Programming/README.md)

### 2. 函数参数位置

```rust
fn print(x: impl Display) {
    println!("{}", x);
}
```

语法糖，等价于匿名泛型：

```rust
fn print<T: Display>(x: T) {
    println!("{}", x);
}
```

同样**静态分发**；差别是无法在签名里给 `T` 命名，也不能在多处复用同一泛型名。

→ 下一节：[10.2 `impl Trait` vs `dyn Trait`](./10-2-impl-vs-dyn.md)
