# 第 19 章 · Strings（字符串） · 内存管理基础（Memory Management）

← [本章目录](./README.md) · 上一节：[01-values-and-objects.md](./01-values-and-objects.md) · 下一节：[03-objstring.md](./03-objstring.md)

---

当前阶段 **无 GC** → **追踪所有分配**，进程结束 **一次性释放**。

```text
allocateObject(size, type):
  obj = reallocate(...)
  obj->next = vm.objects
  vm.objects = obj

freeObjects():
  walk vm.objects 链表 → free 每个 ObjString 等
```

| 设计 | 原因 |
|------|------|
| **侵入式链表 `next`** | 每个 Obj 自带链表指针 · 无单独容器 |
| **退出时释放** | ch26 前足够；泄漏仅存在于 REPL 长会话 |
| **为 GC 铺垫** | 遍历 **`vm.objects`** = 未来 **mark 根集合** 的起点之一 |

**对照 RFR / 系统编程**：堆分配 · 所有权；clox 最终由 **GC** 接管，而非 Rust 式静态所有权。

---
