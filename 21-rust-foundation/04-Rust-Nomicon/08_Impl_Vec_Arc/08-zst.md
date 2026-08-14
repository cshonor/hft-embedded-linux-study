# 8 · 零大小类型 (ZST)

← [本章目录](./README.md) · 上一节：[07-iterators.md](./07-iterators.md) · 下一节：[01-arc-overview.md](./01-arc-overview.md)

---

| 问题 | 对策 |
|------|------|
| alloc(0) → UB | 不调用分配器；`cap = usize::MAX` |
| `ptr.offset` 为 no-op | 迭代用 **usize 计数**，指针 cast 推进 |
| 读 ZST | 从 **`NonNull::dangling()`** 对齐地址 read |
| Drop RawVec | ZST 跳过 `dealloc` |

→ 源码：[src/raw_vec.rs](./src/raw_vec.rs) · [src/zst.rs](./src/zst.rs)
