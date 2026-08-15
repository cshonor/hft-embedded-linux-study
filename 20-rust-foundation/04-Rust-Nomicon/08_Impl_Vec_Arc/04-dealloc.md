# 4 · 内存释放 (Dealloc)

← [本章目录](./README.md) · 上一节：[03-push-pop.md](./03-push-pop.md) · 下一节：[05-deref.md](./05-deref.md)

---

`Drop`：循环 `pop` 清理元素（无 Drop 的类型可被优化掉）→ `dealloc`；`cap == 0` 或未 alloc 则跳过。
