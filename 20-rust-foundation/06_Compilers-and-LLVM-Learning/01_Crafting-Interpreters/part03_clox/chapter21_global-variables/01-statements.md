# 第 21 章 · Global Variables（全局变量） · 语句与状态效果（Statements）

← [本章目录](./README.md) · 上一节：[00-overview.md](./00-overview.md) · 下一节：[02-variable-declarations.md](./02-variable-declarations.md)

---

| 语句 | 编译概要 |
|------|----------|
| **`print expr;`** | compile `expr` → **`OP_PRINT`** |
| **`expr;`** | compile `expr` → **`OP_POP`**（丢弃栈顶） |

**栈效应为 0**：

- 语句执行后 **栈深度与进入前相同**。
- 表达式求值会 **push 结果**；纯副作用语句 **不应留垃圾**。

```text
expression statement:
  ... compile expr ...   // stack: [..., result]
  OP_POP                 // stack: [...]
```

**对照 jlox [ch8](../../part02_jlox/chapter08_statements-and-state/README.md)**：表达式语句 Visitor 直接 **忽略返回值**；clox 显式 **POP**。

---
