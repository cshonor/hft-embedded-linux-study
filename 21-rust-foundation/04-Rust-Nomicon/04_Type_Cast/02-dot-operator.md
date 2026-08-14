# 2 · 点运算符魔法 (The Dot Operator)

← [本章目录](./README.md) · 上一节：[01-coercions.md](./01-coercions.md) · 下一节：[03-casts.md](./03-casts.md)

---

`value.foo()` 时，编译器按优先级尝试直到匹配：

1. **按值**调用 `T::foo`
2. **Auto-referencing**：`&T` / `&mut T`
3. **Auto-dereferencing**：`Deref` / `DerefMut` 链
4. **Unsizing**：如 `[T; N]` → `[T]`，以寻找方法

→ 源码：[src/dot_operator.rs](./src/dot_operator.rs)
