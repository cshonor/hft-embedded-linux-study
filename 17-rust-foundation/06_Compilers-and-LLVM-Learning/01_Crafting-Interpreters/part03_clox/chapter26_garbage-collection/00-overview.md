# 第 26 章 · Garbage Collection（垃圾回收） · 本章定位

← [本章目录](./README.md) · 下一节：[01-marking-tri-color-abstraction.md](./01-marking-tri-color-abstraction.md)

---

ch19 起 **堆分配**（字符串、函数、闭包、Upvalue…）仅靠 **进程退出 `freeObjects`** 不够 → 手写 **Mark-Sweep GC**，替换 ch19 的朴素释放。

| 对比 | ch19 | ch26 |
|------|------|------|
| 释放 | **`freeObjects()` 退出时** | **可达性 GC** |
| 追踪 | 侵入式 **`vm.objects` 链表** | 同链表 + **mark 位** |
| 字符串表 | intern 表 | **弱引用** 特殊清扫 |

| 主题 | 要点 |
|------|------|
| **Mark** | 根集合 · **三色抽象** |
| **Sweep** | 白对象 **free** |
| **Weak refs** | **`vm.strings`** 与 GC 协同 |
| **触发** | **自适应阈值** |

---
