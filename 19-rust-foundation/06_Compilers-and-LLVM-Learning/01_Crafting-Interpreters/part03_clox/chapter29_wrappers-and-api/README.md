# 第 29 章 · Superclasses（超类与继承）

> **Crafting Interpreters** · [Part III · clox](../README.md) · [本书目录](../../本书目录.md)  
> 在线：[craftinginterpreters.com](https://craftinginterpreters.com/wrappers-and-api.html) · [中文在线](https://craftinginterpreters.com/methods.html)

## 状态

- [x] 已读（笔记整理）

---

## 一句话

OOP 最后一块：`<` 继承 · copy-down 方法表 · `super` 静态绑定 · `OP_SUPER_INVOKE`。

---

## 专项笔记（一小节一文件）

| 小节 | 主题 | 阅读 |
|------|------|------|
| — | 本章定位 | [00-overview.md](./00-overview.md) |
| §29.1 | 继承（Inheritance） | [01-inheritance.md](./01-inheritance.md) |
| §29.2 | 超类局部变量（A Superclass Local Variable） | [02-a-superclass-local-variable.md](./02-a-superclass-local-variable.md) |
| §29.3～ | §29.4 Super 表达式与优化 | [03-29-4-super.md](./03-29-4-super.md) |
| ·5 | 继承 + super 数据流 | [04-super.md](./04-super.md) |
| — | 速记 · 自测 · 进度 |

---

## 逻辑脉络


---

## 速记

## 本章速记

```text
§29.1  < Super · OP_INHERIT copy-down · 一次 hash 查方法
§29.2  super 局部/upvalue · 超类 ObjClass*
§29.3  OP_GET_SUPER · 从超类表查 · 非子类
§29.4  OP_SUPER_INVOKE · 融合 super+调用
原书29  Wrappers and API · REPL/C 嵌入 · 另读官网
```

---

---

## 读后下一步

| 章 | 目录 | 内容 |
|:--:|------|------|
| **30** | [chapter30 · Optimization](../chapter30_optimization/) | **`& (cap-1)`** · **NaN boxing** |
| **29 原书** | [Wrappers and API](https://craftinginterpreters.com/wrappers-and-api.html) | **`run()` REPL** · embed API |
| **13** jlox | Inheritance | 超类链 vs copy-down 对照 |

---

---

## 自测

1. copy-down 后，子类还能否调用被 override 的超类方法？（通过什么？）
2. `OP_GET_SUPER` 为何不能等价于 `OP_GET_PROPERTY`？
3. `OP_SUPER_INVOKE` 相对 super Get+Call 省什么？

---

---

## 阅读进度

- [x] §29.1～§29.4 结构梳理（本章笔记）
- [ ] 对照 jlox ch13 画 copy-down vs 链式查找
- [ ] 阅读原书 Wrappers and API（可选）

