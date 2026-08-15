# 4 · 内存重释 (Transmutes)

← [本章目录](./README.md) · 上一节：[03-casts.md](./03-casts.md)

---

`mem::transmute<T, U>` — **终极黑魔法**：强制把底层比特 reinterpret 为另一类型，**唯一要求**是 `size_of` 相同。

- 作者称此为 Rust 中**最可怕、最不安全**的操作之一，防护形同虚设。
- 误用 → 立即 **UB**。
- **`&T` transmute 成 `&mut T` 永远是 UB**（破坏别名与优化假设）。

→ 源码：[src/transmute.rs](./src/transmute.rs)（仅演示同尺寸无害 reinterpret；危险用法仅注释）
