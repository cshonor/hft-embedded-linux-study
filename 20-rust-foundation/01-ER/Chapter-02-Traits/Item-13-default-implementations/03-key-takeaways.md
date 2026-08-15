# Item 13 · 重点结论

← [Item 13 目录](./README.md)

1. **小强制面 + 大可用面**——学标准库：`Iterator` 只强制 `next()`，赠送 50+ 方法。
2. **`where` 做条件增强**——如 `cloned()` 仅在 `T: Clone` 时可用，不增加基元 impl 负担。
3. **公共库 API 演进**——优先用**默认实现**扩展 trait，而非逼所有下游改 impl。
4. 与 **Item 12**：默认方法若含泛型 / `Self`，用 **`where Self: Sized`** 保护 **对象安全**（见 §6）。

---
