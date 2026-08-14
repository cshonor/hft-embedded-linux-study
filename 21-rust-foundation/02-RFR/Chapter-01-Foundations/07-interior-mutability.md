# 3.3 Interior Mutability（内部可变性）

> 所属：**Borrowing and Lifetimes** · [← 章索引](./README.md)

← [06 可变引用](./06-mutable-references.md) · 下一节 → [08 生命周期](./08-lifetimes.md)

---

**内部可变性**：外层常用 `let` / `&self`，通过 `Cell` / `RefCell` / `Mutex` 等容器修改**盒内**数据；检查从编译期部分下沉到运行时（或锁）。

```text
外部可变性（let mut / &mut）     编译期互斥，默认首选
        ↓ 静态规则不够时
内部可变性（UnsafeCell 底层）   外层绑定不动，盒内受控修改
        ↓
单线程：Cell（Copy）· RefCell（通用）
多线程：Mutex · RwLock
```

---

## 子节导航

| § | 主题 | 阅读 |
|---|------|------|
| **07.1** | 外部可变性 vs 内部可变性 | [07-1-external-vs-interior.md](./07-1-external-vs-interior.md) |
| **07.2** | `UnsafeCell` 与容器速查 | [07-2-unsafecell-and-containers.md](./07-2-unsafecell-and-containers.md) |
| **07.3** | `Cell` 与 `RefCell` 详解 | [07-3-cell-vs-refcell.md](./07-3-cell-vs-refcell.md) |
| **07.4** | 应用场景 | [07-4-use-cases.md](./07-4-use-cases.md) |
| **07.5** | 对比 · 误区 · 总纲 | [07-5-comparison-pitfalls.md](./07-5-comparison-pitfalls.md) |
| — | 速记 · 自测 |

**建议阅读顺序**：`07.1` → `07.2` → `07.3` → `07.4` → `07.5`

---

## 一句话选型

> **能 `let mut` 就用外部可变；`&self` / `Rc` / 细粒度字段 → 内部可变。小 Copy 用 `Cell`，复杂 `T` 用 `RefCell`，多线程用 `Mutex`。**

---

## 延伸阅读

- 前置 → [05 共享引用](./05-shared-references.md) · [06 可变引用](./06-mutable-references.md)
- 下一节 → [08 生命周期](./08-lifetimes.md)
- 全章速记 → 