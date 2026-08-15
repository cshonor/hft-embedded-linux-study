# 5 · 数据投毒 (Poisoning)

← [本章目录](./README.md) · 上一节：[04-unwinding.md](./04-unwinding.md)

---

无法在 panic 后恢复完美逻辑一致性时，用**投毒**阻断坏状态蔓延。例：`Mutex` 持锁期间 panic → 锁被 **poison** → 后续 `lock()` 默认失败。

→ 源码：[src/poisoning.rs](./src/poisoning.rs)
