# 2 · 内存泄漏与 `mem::forget` (Leaking)

← [本章目录](./README.md) · 上一节：[01-construct-drop.md](./01-construct-drop.md) · 下一节：[03-proxy-types.md](./03-proxy-types.md)

---

Safe Rust **并非**绝对无泄漏——死锁、`Rc` 循环引用等均可导致资源永不释放。

`mem::forget` 消耗值但**不调用 Drop**；官方认定这不违背**内存安全**（只是泄漏资源）。

→ 源码：[src/forget_leak.rs](./src/forget_leak.rs)
