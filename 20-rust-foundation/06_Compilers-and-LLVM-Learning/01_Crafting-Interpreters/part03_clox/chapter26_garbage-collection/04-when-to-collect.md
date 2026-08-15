# 第 26 章 · Garbage Collection（垃圾回收） · 触发回收（When to Collect）

← [本章目录](./README.md) · 上一节：[03-weak-references.md](./03-weak-references.md) · 下一节：[05-gc-clox.md](./05-gc-clox.md)

---

**不能** 每次 `allocate` 都 GC（太慢）→ **字节阈值** + **自适应**。

| 机制 | 说明 |
|------|------|
| **`bytesAllocated`** | 累计分配量 |
| **`nextGC`** | 下次触发阈值 |
| **`collectGarbage()`** | 超阈值 → mark + sweep |
| **自适应** | GC 后根据 **存活 bytes** 调整 **`nextGC`**（如 **`alive * GC_HEAP_GROW_FACTOR`**） |

**平衡**：存活对象多 → 阈值升高，减少 GC 频率；存活少 → 更勤回收，控内存。

**REPL / 长运行**：ch19 退出才 free → ch26 后 **可持续交互**。

---
