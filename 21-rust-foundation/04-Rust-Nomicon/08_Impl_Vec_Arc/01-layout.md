# 1 · 数据布局 (Layout)

← [本章目录](./README.md) · [00-overview.md](./00-overview.md) · 下一节：[02-allocating.md](./02-allocating.md)

---

`Vec` = **指针 + cap + len**。stable 路径用 **`NonNull<T>`**（协变、非空保证、支持 niche）。因非 `Unique`，`T: Send/Sync` 时需 **`unsafe impl Send/Sync`**。

→ 源码：[src/my_vec.rs](./src/my_vec.rs)
