# 4 · 与后续章节的关系

← [本章目录](./README.md) · 上一节：[03-unchecked.md](./03-unchecked.md)

---

`Vec` 的容量增长、部分提交元素，是本章 API 的典型组合：`MaybeUninit` 数组 + `ptr::write` + `set_len` → 见 [08_Impl_Vec_Arc](../08_Impl_Vec_Arc/README.md)。
