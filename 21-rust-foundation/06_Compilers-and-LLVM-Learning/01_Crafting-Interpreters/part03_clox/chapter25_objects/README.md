# 第 25 章 · Closures（闭包）

> **Crafting Interpreters** · [Part III · clox](../README.md) · [本书目录](../../本书目录.md)  
> 在线：[craftinginterpreters.com](https://craftinginterpreters.com/calling-and-closures.html) · [中文在线](https://craftinginterpreters.com/calling-and-closures.html)

## 状态

- [x] 已读（笔记整理）

---

## 一句话

jlox 闭包靠 Java GC + Environment 捕获；clox 局部变量在 栈槽 上，函数返回即 弹栈销毁 → 闭包若仍指向栈地址 = 悬空指针。

---

## 专项笔记（一小节一文件）

| 小节 | 主题 | 阅读 |
|------|------|------|
| — | 本章定位 | [00-overview.md](./00-overview.md) |
| ·2 | Upvalue（上值） | [01-closures-upvalue.md](./01-closures-upvalue.md) |
| ·3 | 扁平化 Upvalues（Flattening Upvalues） | [02-flattening-upvalues.md](./02-flattening-upvalues.md) |
| ·4 | 闭合 Upvalue（Closing Upvalues） | [03-closing-upvalues.md](./03-closing-upvalues.md) |
| ·5 | ObjClosure 与 OP_CLOSURE | [04-objclosure-op-closure.md](./04-objclosure-op-closure.md) |
| ·6 | 闭包生命周期（总览） | [05-closures.md](./05-closures.md) |
| — | 速记 · 自测 · 进度 |

---

## 逻辑脉络


---

## 速记

## 本章速记

```text
Upvalue     间接层 · ObjClosure · GET/SET_UPVALUE
Flatten     嵌套时中间层传递 upvalue 索引
Close       OP_CLOSE_UPVALUE · open→closed · 防悬空
对照 jlox   Environment+distance ↔ upvalue+slot
```

---

---

## 读后下一步

| 章 | 目录 | 内容 |
|:--:|------|------|
| **26** | [chapter26 · GC](../chapter26_garbage-collection/) | **Mark-Sweep** · 三色 |
| **27** | [chapter27 · Classes](../chapter27_classes-and-instances/) | **`ObjClass`** · 实例 |
| **28** | Methods | **`this`** · 方法绑定 |

---

---

## 自测

1. 为何不能 let 闭包长期持有「栈 slot 指针」而不 close？
2. Flattening 时 middle 不捕获 x，inner 会怎样？
3. `ObjClosure` 与 `ObjFunction` 在 `OP_CALL` 路径上的差别？

---

---

## 阅读进度

- [x] Upvalue / Flatten / Close 结构梳理（本章笔记）
- [ ]  trace `outer()` return 后 inner 读 x 的 upvalue 状态
- [ ] 原书 ch24 闭包 Challenges

