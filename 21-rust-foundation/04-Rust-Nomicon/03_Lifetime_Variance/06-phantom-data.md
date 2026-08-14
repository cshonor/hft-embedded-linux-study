# 6 · 幽灵数据（PhantomData）

← [本章目录](./README.md) · 上一节：[05-drop-check.md](./05-drop-check.md) · 下一节：[07-split-borrows.md](./07-split-borrows.md)

---

类型在逻辑上「拥有」某泛型/生命周期，但物理字段中**没有**存储它。须用 **`PhantomData`** 零大小标记，向型变与 Drop Check 传递正确语义，避免编译器错误推断。

→ 源码：[src/phantom.rs](./src/phantom.rs)
