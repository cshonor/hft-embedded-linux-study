# Item 9 · 重点结论

← [Item 9 目录](./README.md)

1. **优先迭代器链**——更惯用、意图更清晰（过滤/映射/聚合各就各位）。
2. **`collect::<Result<Vec<_>, _>>()`**——把 `Iterator<Item = Result<T,E>>` 收成 **`Result<Vec<T>, E>`**；首个 `Err` 即停；配合 `?` 向上传。
3. **何时保留显式 `for`**——循环体巨大、多职责、需复杂 early return 时，硬塞进闭包反而难读。
4. **别盲目迷信**——热点路径用 `cargo bench` / Godbolt 对比显式循环与迭代器。

---
