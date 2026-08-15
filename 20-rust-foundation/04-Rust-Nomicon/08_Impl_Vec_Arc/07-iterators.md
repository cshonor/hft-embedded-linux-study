# 7 · IntoIter、Drain 与 RawVec

← [本章目录](./README.md) · 上一节：[06-insert-remove.md](./06-insert-remove.md) · 下一节：[08-zst.md](./08-zst.md)

---

| 组件 | 职责 |
|------|------|
| **`RawVec`** | 指针 + cap；分配/释放逻辑复用 |
| **`IntoIter`** | 双指针 `start`/`end`；`ptr::read` + 推进 |
| **`Drain`** | 借用 Vec；**初始化时 `len = 0`** 防泄漏放大 / panic 不安全 |

→ 源码：[src/raw_vec.rs](./src/raw_vec.rs) · [src/into_iter.rs](./src/into_iter.rs) · [src/drain.rs](./src/drain.rs)
