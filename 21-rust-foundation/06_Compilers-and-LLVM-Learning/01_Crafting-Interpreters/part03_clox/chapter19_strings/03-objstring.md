# 第 19 章 · Strings（字符串） · ObjString 要点

← [本章目录](./README.md) · 上一节：[02-memory-management.md](./02-memory-management.md) · 下一节：---

| 字段 | 作用 |
|------|------|
| **`length`** | 非 NUL 依赖长度（可含 `\0`） |
| **`chars`** | 堆缓冲区，通常 **`length+1`** 结尾 `\0` 便于 `printf` |
| **`hash`** | 预计算 **FNV-1a**（ch20 表键 · intern 用） |

**运行时**：字符串 **拼接、打印** 等在后续章节扩展；本章重点是 **表示 + 分配 + 链表**。

---
