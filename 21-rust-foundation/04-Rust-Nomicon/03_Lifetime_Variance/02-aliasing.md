# 2 · 别名（Aliasing）的本质与优化

← [本章目录](./README.md) · 上一节：[01-ownership-basics.md](./01-ownership-basics.md) · 下一节：[03-lifetimes.md](./03-lifetimes.md)

---

**别名**：多个变量/指针指向重叠内存。

因 Rust 严格限制 `&mut` 别名，编译器可激进优化（寄存器缓存、消除冗余读写、指令重排），而不必担心其它指针暗中改数据——这在许多语言中很难做到。
