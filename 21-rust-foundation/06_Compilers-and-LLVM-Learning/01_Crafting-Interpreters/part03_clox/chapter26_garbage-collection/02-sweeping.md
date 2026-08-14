# 第 26 章 · Garbage Collection（垃圾回收） · 清除阶段（Sweeping）

← [本章目录](./README.md) · 上一节：[01-marking-tri-color-abstraction.md](./01-marking-tri-color-abstraction.md) · 下一节：[03-weak-references.md](./03-weak-references.md)

---

**Mark 完成后**：

```text
for obj in vm.objects 链表:
  if !obj.isMarked:
    free(obj)           // 白 → 垃圾
  else:
    obj.isMarked = false  // 清除 mark 位，下轮 GC 用
```

| 要点 | 说明 |
|------|------|
| **Sweep 线性扫堆链表** | 与 allocate 时链接的 **`vm.objects`** 一致 |
| **黑色/存活** | 保留并 **unmark** |
| **白色** | **`reallocate(..., 0)` 释放** |

**碎片**：Mark-Sweep **不移动对象**（非 compacting）；简单，适合教学 VM。

---
