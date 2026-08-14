# 第 26 章 · Garbage Collection（垃圾回收） · 弱引用与字符串池（Weak References）

← [本章目录](./README.md) · 上一节：[02-sweeping.md](./02-sweeping.md) · 下一节：[04-when-to-collect.md](./04-when-to-collect.md)

---

**问题**：**`vm.strings`** intern 表 **强引用** 字符串 → 程序中已无用的字符串 **仍被表钉住** → 假存活；若表不更新，还可能 **悬空**（若别处误 free）。

**处理**（书中策略概要）：

| 阶段 | 对 intern 表 |
|------|--------------|
| **Mark 后 / Sweep 前** | 表作为 **弱引用**：**移除** 未被 mark 的字符串条目 |
| **Sweep** | 只 free **堆链表上未 mark 的 Obj** |

**效果**：intern 表 **不阻止** 字符串被回收；相等 intern 语义在 **存活期** 内仍有效。

**对照 ch20 [intern](../chapter20_hash-tables/README.md)**：性能优化与 GC **必须协同设计**。

---
