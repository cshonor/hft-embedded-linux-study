# 第 28 章 · Methods and Initializers（方法与初始化器）

> **Crafting Interpreters** · [Part III · clox](../README.md) · [本书目录](../../本书目录.md)  
> 在线：[craftinginterpreters.com](https://craftinginterpreters.com/methods.html) · [中文在线](https://craftinginterpreters.com/methods.html)

## 状态

- [x] 已读（笔记整理）

---

## 一句话

为 OOP 注入可调用行为：类方法表 · `this` 作 slot 0 · `init` · `OP_INVOKE` 超指令 避免 BoundMethod 堆分配。

---

## 专项笔记（一小节一文件）

| 小节 | 主题 | 阅读 |
|------|------|------|
| — | 本章定位 | [00-overview.md](./00-overview.md) |
| §28.1 | 方法声明（Method Declarations） | [01-method-declarations.md](./01-method-declarations.md) |
| §28.2 | 绑定方法（Bound Methods） | [02-bound-methods.md](./02-bound-methods.md) |
| §28.3 | This 关键字（This） | [03-this.md](./03-this.md) |
| §28.4 | 初始化器（Initializers） | [04-initializers.md](./04-initializers.md) |
| ·6 | Superinstruction） | [05-optimized-invocations.md](./05-optimized-invocations.md) |
| ·7 | 方法调用路径（小结） | [06-methods-and-initializers.md](./06-methods-and-initializers.md) |
| — | 速记 · 自测 · 进度 |

---

## 逻辑脉络


---

## 速记

## 本章速记

```text
§28.1  class.methods Table · ObjClosure
§28.2  ObjBoundMethod · receiver+method
§28.3  this = slot 0 · GET/SET_LOCAL 0
§28.4  init 自动调用 · 只 return this
§28.5  OP_INVOKE 融合查+调 · 免堆分配
```

---

---

## 读后下一步

| 章 | 目录 | 内容 |
|:--:|------|------|
| **29** | [chapter29 · Superclasses](../chapter29_wrappers-and-api/README.md) | **`OP_INHERIT`** · **`super`** |
| **30** | [chapter30 · Optimization](../chapter30_optimization/) | 掩码 · **NaN boxing** |

---

---

## 自测

1. 为何 `this` 用 slot 0 而不是单独 `OP_THIS`？
2. `OP_INVOKE` 相对 Get+Call 省掉了哪次堆分配？
3. 实例字段与方法同名时，`OP_INVOKE` 应命中哪一个？

---

---

## 阅读进度

- [x] §28.1～§28.5 结构梳理（本章笔记）
- [ ] 对照 benchmark 理解 invoke 提升
- [ ] 本章 Challenges

