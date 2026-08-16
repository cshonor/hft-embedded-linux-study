# 2.5.1 · 标记 Trait 基础

> 所属：**Traits and Trait Bounds · 标记 Trait** · [← 09 hub](./09-marker-traits.md)

← [08.3 示例与避坑](./08-3-examples-pitfalls.md) · 下一节 [09.2 Sized 与 Copy](./09-2-sized-copy.md)

---

标记 Trait（Marker Trait）是一类**不含方法 / 关联常量、仅承载语义标记**的特殊 trait。向编译器传递类型的安全属性、内存布局属性，约束编译检查、指导优化。

典型代表：`Sized`、`Copy`、`Send`、`Sync`、`Unpin`。

---

## 与普通 Trait 的区别

| | **Marker Trait** | **普通 Trait** |
|---|------------------|----------------|
| 方法 | 无（或几乎无） | 有 `fn` 等多态行为 |
| 用途 | 编译器契约、类型检查 | API 行为抽象 |
| 运行时 | **零开销**（编译期完成） | 静态或动态分发 |
| `impl` 约束 | `Send`/`Sync` 等可 `unsafe impl` | 通常 safe `impl` |
| 孤儿规则 | **同样适用** | 同样适用 |

→ 下一节：[09.2 `Sized` 与 `Copy`](./09-2-sized-copy.md)
