# 2.1 Compilation and Dispatch（编译与分发）

> 所属：**Traits and Trait Bounds** · [← 章索引](./README.md)

← [04 DST 与宽指针](./04-dst-wide-pointers.md) · 下一节 → [06 泛型 Trait](./06-generic-traits.md)

---

Trait 定义类型间的**可替换行为契约**；编译器如何把方法调用变成机器码，就是**分发模型**。

```text
静态（单态化）  编译期绑定 · 无 vtable · HFT 热路径首选
动态（dyn）     运行时 vtable · 异构集合 · 有间接调用开销
```

前置 → [04 DST](./04-dst-wide-pointers.md)（`dyn Trait` 载体）· 存在类型 → [10 impl Trait](./10-existential-types.md)

---

## 子节导航

| § | 主题 | 阅读 |
|---|------|------|
| **05.1** | 静态 vs 动态分发 · 单态化深度 | [05-1-static-vs-dynamic.md](./05-1-static-vs-dynamic.md) |
| **05.2** | 单态化与内存布局 | [05-2-monomorphization-memory.md](./05-2-monomorphization-memory.md) |
| **05.3** | 对象安全 (Object Safety) | [05-3-object-safety.md](./05-3-object-safety.md) |
| **05.4** | 选型 · HFT · 汇编直觉 | [05-4-selection-hft.md](./05-4-selection-hft.md) |
| — | 速记 · 自测 |

**建议阅读顺序**：`05.1` → `05.2` → `05.3` → `05.4`

---

## 一句话选型

> **热路径 `T: Trait` / enum；异构集合 `dyn`；返回复杂类型 `impl Trait`（仍静态）。**

---

## 延伸阅读

- Trait 限定语法 → [08 Trait 限定](./08-trait-bounds.md)
- ER → [Item 12 泛型 vs trait object](../../01-ER/Chapter-02-Traits/Item-12-generics-vs-trait-objects/README.md)
