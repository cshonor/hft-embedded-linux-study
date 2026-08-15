# 2.2 Object Safety（对象安全）

> 所属：**Flexible** · [← 章索引](./README.md)

设计 Trait 时考虑**能否作为 `dyn Trait` 使用**，便于异构集合与插件式架构。

## 为什么重要

- `Vec<Box<dyn Handler>>` 等模式依赖对象安全。
- 非热点路径用 **`dyn Trait`** 可减小二进制与编译时间 → [05 编译与分发](../Chapter-02-Types/05-compilation-dispatch.md)

## 常见非对象安全原因

- 泛型方法 `fn foo<T>(&self, x: T)`
- 返回 `Self` 且与对象大小绑定
- 需要 `Self: Sized` 的方法等

## 设计技巧

- 把泛型方法拆成 **关联类型** 或 **独立 trait**。
- 用 `where Self: Sized` 的 **default impl** 保留泛型便利，同时保留 `dyn` 可用性。

ER → [Item 12 · 对象安全](../../01-ER/Chapter-02-Traits/Item-12-generics-vs-trait-objects/README.md) · Book → [17.2 trait 对象](../../00-Book/17-oop/17.2-为使用不同类型的值而设计的trait对象.md)
