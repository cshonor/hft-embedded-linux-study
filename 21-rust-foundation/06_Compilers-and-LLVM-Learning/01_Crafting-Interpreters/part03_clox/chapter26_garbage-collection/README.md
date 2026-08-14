# 第 26 章 · Garbage Collection（垃圾回收）

> **Crafting Interpreters** · [Part III · clox](../README.md) · [本书目录](../../本书目录.md)  
> 在线：[craftinginterpreters.com](https://craftinginterpreters.com/garbage-collection.html) · [中文在线](https://craftinginterpreters.com/garbage-collection.html)

## 状态

- [x] 已读（笔记整理）

---

## 一句话

ch19 起 堆分配（字符串、函数、闭包、Upvalue…）仅靠 进程退出 `freeObjects` 不够 → 手写 Mark-Sweep GC，替换 ch19 的朴素释放。

---

## 专项笔记（一小节一文件）

| 小节 | 主题 | 阅读 |
|------|------|------|
| — | 本章定位 | [00-overview.md](./00-overview.md) |
| ·2 | 标记阶段与三色抽象（Marking & Tri-color Abstraction） | [01-marking-tri-color-abstraction.md](./01-marking-tri-color-abstraction.md) |
| ·3 | 清除阶段（Sweeping） | [02-sweeping.md](./02-sweeping.md) |
| ·4 | 弱引用与字符串池（Weak References） | [03-weak-references.md](./03-weak-references.md) |
| ·5 | 触发回收（When to Collect） | [04-when-to-collect.md](./04-when-to-collect.md) |
| ·6 | GC 与 clox 子系统 | [05-gc-clox.md](./05-gc-clox.md) |
| — | 速记 · 自测 · 进度 |

---

## 逻辑脉络


---

## 速记

## 本章速记

```text
Mark      根 → 三色/work 栈 → markObject 图遍历
Sweep     链表扫未 mark → free · 清除 mark 位
Strings   intern 表弱引用 · sweep 间清理
Trigger   bytesAllocated vs nextGC · 自适应阈值
```

---

---

## 读后下一步

| 章 | 目录 | 内容 |
|:--:|------|------|
| **27** | [chapter27 · Classes](../chapter27_classes-and-instances/) | **`OP_CLASS`** · **`ObjInstance`** |
| **28** | Methods | 方法 · **`this`** |
| **19** ch19 | Strings | 对比 exit-time free → GC |

---

---

## 自测

1. 闭包循环引用两个 Obj，无根可达时 GC 能否回收？
2. 为何 intern 表不能对字符串做普通强 mark？
3. `nextGC` 自适应调大有什么好处与风险？

---

---

## 阅读进度

- [x] Mark-Sweep / 三色 / 弱引用 / 触发 结构梳理（本章笔记）
- [ ] 画一轮 GC 前后 objects 链表与 strings 表
- [ ] 本章 Challenges

