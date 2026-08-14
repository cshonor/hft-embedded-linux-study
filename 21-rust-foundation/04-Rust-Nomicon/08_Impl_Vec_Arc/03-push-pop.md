# 3 · Push 与 Pop

← [本章目录](./README.md) · 上一节：[02-allocating.md](./02-allocating.md) · 下一节：[04-dealloc.md](./04-dealloc.md)

---

| 操作 | API | 原因 |
|------|-----|------|
| **push** | `ptr::write` | 普通 `*ptr = x` 会先 Drop **未初始化**旧比特 → UB |
| **pop** | `ptr::read` | 不能直接 move 出（会留下逻辑未初始化槽位） |
