# 3.2 · `impl Trait` vs `dyn Trait`

> 所属：**Existential Types** · [← 10 hub](./10-existential-types.md)

← [10.1 存在量化与位置](./10-1-logic-positions.md) · 下一节 [10.3 限制与选型](./10-3-limits-selection.md)

---

## 一、对比表

| 对比维度 | `impl Trait`（存在类型） | `dyn Trait`（trait 对象） |
|----------|--------------------------|---------------------------|
| 分发机制 | 静态分发，编译单态化 | 动态分发，运行时查 vtable |
| 内存大小 | 编译期确定（`Sized`） | DST，须 `Box` / `&` 等宽指针 |
| 异构集合 | 不支持；元素须同一底层类型 | 支持 `Vec<Box<dyn Trait>>` |
| 类型可见性 | 编译器知真实类型，调用方不可见 | 擦除具体类型，仅保留 vtable |
| 运行开销 | 无额外开销 | 微小间接调用开销 |

→ [05 hub](./05-compilation-dispatch.md) · [04.3 dyn/vtable](./04-3-dyn-vtable.md)

---

## 二、典型使用场景

| 场景 | 为什么用 `impl Trait` |
|------|------------------------|
| **长迭代器 / 闭包链** | 匿名闭包拼接类型名不可维护 |
| **`async fn`** | 匿名 `Future` 结构体无法手写 |
| **库 API 封装** | 内部换实现不破坏对外签名 |
| **简化泛型** | 单参数、无复用 `T` 时少写 `<T: Bound>` |

```rust
// 库内部可换实现，签名稳定
pub fn load_config() -> impl Read {
    std::fs::File::open("app.toml").unwrap()
}
```

→ 下一节：[10.3 核心限制与选型](./10-3-limits-selection.md)
