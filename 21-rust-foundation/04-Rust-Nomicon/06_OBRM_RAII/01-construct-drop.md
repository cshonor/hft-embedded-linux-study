# 1 · 构造与析构 (Constructors and Destructors)

← [本章目录](./README.md) · [00.1 RAII/OBRM](./00-1-RAII与OBRM辨析.md) · [00-overview.md](./00-overview.md) · 下一节：[02-forget-leak.md](./02-forget-leak.md)

---

## 极简构造

无 C++ 式拷贝/默认/移动构造分族；类型不「关心」自己在哪块内存——移动只是 **memcpy**，故无法安全实现纯栈上**侵入式链表**等须感知地址的结构。

## 递归析构

`Drop` 自动析构；`drop` 方法体执行后**递归**清理各字段。可用 **`Option::take`** 或 **`ManuallyDrop`** 打破/延迟默认递归清理。

→ 源码：[src/construct_drop.rs](./src/construct_drop.rs)
