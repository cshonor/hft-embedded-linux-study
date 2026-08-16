# 第 13 章 · Inheritance（继承） · §13.4 结论（Conclusion）

← [本章目录](./README.md) · 上一节：[03-calling-superclass-methods.md](./03-calling-superclass-methods.md) · 下一节：[05-ch4-13.md](./05-ch4-13.md)

---

**jlox 管线（完整）**：

```text
Source → Scanner → Parser → AST
              → Resolver（语义分析）
              → Interpreter（Visitor 树遍历）
```

| 已实现（Part II） | 技术要点 |
|------------------|----------|
| 表达式 / 语句 / 控制流 | Visitor · Environment 链 |
| 函数 / 闭包 | `LoxFunction` · closure · distance |
| 类 / 实例 / `this` / `init` | `LoxBoundMethod` |
| **继承 / `super`** | 超类链 · `Expr.Super` |

**为何转向 clox？**

- AST 树遍历：**指针追逐多、缓存不友好、间接层厚**。
- Part III 用 **C + 字节码 + 栈式 VM**：更紧凑、更快、更贴近「真实编译器 / VM」实现。

**下一章**：[ch14 Chunks of Bytecode](../../part03_clox/chapter14_chunks-of-bytecode/) — Part III 起点。

---
