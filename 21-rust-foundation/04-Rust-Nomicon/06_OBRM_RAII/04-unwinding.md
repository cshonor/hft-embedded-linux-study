# 4 · 栈展开与异常安全 (Unwinding)

← [本章目录](./README.md) · 上一节：[03-proxy-types.md](./03-proxy-types.md) · 下一节：[05-poisoning.md](./05-poisoning.md)

---

`panic!` 默认**栈展开**，沿途 Drop 所有对象。Unsafe 代码常调用用户提供的 `Ord`/`Clone` 等——**随时可能 panic**。

须保证至少 **minimal exception safety**：无论如何 panic，**不破坏内存安全**。

对策：RAII **Guard / Hole**——处理中途 panic 时由 Drop 恢复状态，避免 **double-drop**。

→ 源码：[src/panic_guard.rs](./src/panic_guard.rs)
