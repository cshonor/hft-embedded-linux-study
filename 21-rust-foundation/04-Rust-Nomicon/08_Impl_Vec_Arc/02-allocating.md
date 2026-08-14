# 2 · 内存分配 (Allocating)

← [本章目录](./README.md) · 上一节：[01-layout.md](./01-layout.md) · 下一节：[03-push-pop.md](./03-push-pop.md)

---

| 要点 | 做法 |
|------|------|
| 延迟分配 | 空 Vec 不 alloc；`NonNull::dangling()` + `cap == 0` |
| OOM | `handle_alloc_error` |
| 容量上限 | 分配字节 ≤ `isize::MAX`（防 GEP inbounds 溢出） |

→ 源码：[src/raw_vec.rs](./src/raw_vec.rs)
