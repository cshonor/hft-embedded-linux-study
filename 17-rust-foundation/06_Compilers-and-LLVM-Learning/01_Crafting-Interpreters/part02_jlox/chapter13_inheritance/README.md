# 第 13 章 · Inheritance（继承）

> **Crafting Interpreters** · [Part II · jlox](../README.md) · [本书目录](../../本书目录.md)  
> 在线：[craftinginterpreters.com](https://craftinginterpreters.com/inheritance.html) · [中文在线](https://craftinginterpreters.com/inheritance.html)

## 状态

- [x] 已读（笔记整理）

---

## 一句话

Part II 收官章：补全 OOP 最后一块——继承与 `super`。至此 jlox（Java 树遍历解释器）功能完备；下一章起进入 Part III · clox（C 字节码 VM）。

---

## 专项笔记（一小节一文件）

| 小节 | 主题 | 阅读 |
|------|------|------|
| — | 本章定位 | [00-overview.md](./00-overview.md) |
| §13.1 | 超类与子类（Superclasses and Subclasses） | [01-superclasses-and-subclasses.md](./01-superclasses-and-subclasses.md) |
| §13.2 | 超类局部变量（A Superclass Local Variable） | [02-a-superclass-local-variable.md](./02-a-superclass-local-variable.md) |
| §13.3 | 调用超类方法（Calling Superclass Methods） | [03-calling-superclass-methods.md](./03-calling-superclass-methods.md) |
| §13.4 | 结论（Conclusion） | [04-conclusion.md](./04-conclusion.md) |
| ·6 | Part II 能力总览（ch4～13） | [05-ch4-13.md](./05-ch4-13.md) |
| — | 速记 · 自测 · 进度 |

---

## 逻辑脉络


---

## 速记

## 本章速记

```text
§13.1  class Sub < Super · LoxClass.superclass · 方法链查找
§13.2  Resolver 在方法内 declare super → 超类 LoxClass
§13.3  Expr.Super · this 不变 · 超类方法 · 误用检查
§13.4  jlox 完结 → Part III clox
```

---

---

## 读后下一步

| 部分 | 章 | 内容 |
|------|:--:|------|
| **III** | **14** | [Chunks of Bytecode](../../part03_clox/chapter14_chunks-of-bytecode/) · **`Chunk`** · 常量池 |
| **III** | **15** | VM · **ip** · 值栈 |
| **III** | **16** | 按需扫描 · 关键字 DFA |

---

---

## 自测

1. 子类 override 方法后，`super.foo()` 解析到哪一层的方法？
2. 为什么 `super` 要像 `this` 一样在类方法作用域里单独绑定？
3. jlox 与 clox 在「表示程序」上的根本差异是什么？

---

---

## 阅读进度

- [x] §13.1～§13.4 结构梳理（本章笔记）
- [ ] 对照 jlox：`LoxClass.findMethod` 与 `visitSuperExpr`
- [ ] 本章 Challenges · 回顾 Part II 总练习

