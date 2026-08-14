# 第 2 章 · Rust 特征小议

> 所属：[03 DeepRustStdLib](../README.md) · 前：[第 1 章 三层架构](../chapter01_std_overview/README.md) · 后：[第 3 章 内存操作](../chapter03_memory_model/README.md)

**本章定位**（原书第 2 章）：**泛型为本** → **安全封装类型** → **五种解封装范式** → 回顾。为第 3 章裸指针与 `MaybeUninit` 做语法过渡。

**原书主线**：2.1 约束层次 · 2.2 Option/Box/Rc/Mutex/Ref 闭环 · 2.3 `?`/Deref/闭包/as_ref/take。

**阅读顺序**（原书）：**2.1 → 2.2 → 2.3 → 2.4**

---

<!-- AUTO:SECTION-INDEX -->

| 节 | 主题 | 笔记 |
|:---:|------|------|
| **2.1** | 泛型小议 | [2.1-generics-overview.md](./2.1-generics-overview.md) |
| **2.1.1** | 基于泛型的函数及 Trait | [2.1.1-generic-functions-and-traits.md](./2.1.1-generic-functions-and-traits.md) |
| **2.1.2** | 泛型约束的层次 | [2.1.2-generic-constraint-layers.md](./2.1.2-generic-constraint-layers.md) |
| **2.2** | Rust 内存安全杂述 | [2.2-memory-safety-overview.md](./2.2-memory-safety-overview.md) |
| **2.3** | 获取封装类型变量的内部变量 | [2.3-extracting-from-wrappers.md](./2.3-extracting-from-wrappers.md) |
| **2.3.1** | 使用 `?` 运算符解封装 | [2.3.1-unwrap-with-try-operator.md](./2.3.1-unwrap-with-try-operator.md) |
| **2.3.2** | 函数调用 + 自动解引用 | [2.3.2-deref-coercion.md](./2.3.2-deref-coercion.md) |
| **2.3.3** | 采用闭包 | [2.3.3-closures-for-inner-access.md](./2.3.3-closures-for-inner-access.md) |
| **2.3.4** | 获取引用 | [2.3.4-getting-references.md](./2.3.4-getting-references.md) |
| **2.3.5** | 获取所有权 | [2.3.5-getting-ownership.md](./2.3.5-getting-ownership.md) |
| **2.4** | 回顾 | [2.4-recap.md](./2.4-recap.md) |

<!-- /AUTO:SECTION-INDEX -->

> 各节 `.md` 为**笔记本体**（可填原书要点、源码、实战）；本 README 仅为索引。

---

## 补充轨道（RFR ↔ `std`，非原书编号）

| 笔记 | 主题 |
|------|------|
| [2.1-core-features-and-std.md](./2.1-core-features-and-std.md) | 所有权与 `std` |
| [2.2-traits-in-stdlib.md](./2.2-traits-in-stdlib.md) | Trait 与 `std` |
| [2.3-lifetimes-in-stdlib.md](./2.3-lifetimes-in-stdlib.md) | 生命周期与 `std` |
| [2.4-closures-iterator-in-stdlib.md](./2.4-closures-iterator-in-stdlib.md) | 闭包 / Iterator |
| [2.5-pattern-matching-in-stdlib.md](./2.5-pattern-matching-in-stdlib.md) | 模式匹配 |
| [2.6-error-handling-in-stdlib.md](./2.6-error-handling-in-stdlib.md) | 错误处理 |
| [2.7-generics-in-stdlib.md](./2.7-generics-in-stdlib.md) | 泛型与 `std` |

## 附录

| 笔记 | 主题 |
|------|------|
| [appendix-design-philosophy.md](./appendix-design-philosophy.md) | 设计哲学 |
| [appendix-reading-stdlib-source.md](./appendix-reading-stdlib-source.md) | 读源码方法 |

---

## 与 RFR / Book 对照

| 原书节 | RFR | The Book |
|--------|-----|----------|
| 2.1 泛型 | [Ch02 Types](../../02-RFR/Chapter-02-Types/README.md) | [10 泛型与 trait](../../00-Book/10-generics-traits-lifetimes/) |
| 2.2 内存安全 | [Ch01 Foundations](../../02-RFR/Chapter-01-Foundations/README.md) | [04 所有权](../../00-Book/04-ownership/) |
| 2.3 解封装 | [Ch04 Error · `?`](../../02-RFR/Chapter-04-Error-Handling/README.md) | [15 智能指针](../../00-Book/15-smart-pointers/) |
