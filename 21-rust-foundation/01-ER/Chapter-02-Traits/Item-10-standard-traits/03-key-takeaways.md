# Item 10 · 重点结论

← [Item 10 目录](./README.md)

1. **`Copy` 要克制**——小、纯数据、按位拷贝便宜；大 struct 的隐式 Copy 可能拖性能；含 `&mut T` 等不能 Copy。
2. **尽量 derive `Debug`**——除非敏感字段；可 `#![warn(missing_debug_implementations)]`。
3. **`Default` + `..Default::default()`**——多可选字段初始化（与 Item 7 Builder 互补）。
4. **`std::ops` 重载要一致**——有 `Add` + `Neg` 则 `Sub` 应满足 `x - y ≈ x + (-y)`；别给无关类型乱重载。
5. **手写 `PartialEq` 时别忘 `Eq`**——若逻辑满足反射性，补 `impl Eq for T {}` 才能当 `HashMap` key。

---
