# 4 · 子类型与型变（Subtyping and Variance）

← [本章目录](./README.md) · 上一节：[03-lifetimes.md](./03-lifetimes.md) · 下一节：[05-drop-check.md](./05-drop-check.md)

---

为允许「长生命周期降级为短生命周期」传递引用，Rust 引入子类型与型变。**全书最硬核理论之一**：

| 型变 | 含义 | Rust 典型 |
|------|------|-----------|
| **协变 (Covariant)** | 子类型方向与类型参数同向 | `&'a T` 对 `'a` 协变；`&'a T` 对 `T` 多数协变 |
| **逆变 (Contravariant)** | 方向相反 | **函数参数**是 Rust 中主要逆变来源 |
| **不变 (Invariant)** | 不允许升降 | `&mut T` 对 `T` **严格不变**（防 use-after-free） |

→ 源码：[src/variance.rs](./src/variance.rs)
