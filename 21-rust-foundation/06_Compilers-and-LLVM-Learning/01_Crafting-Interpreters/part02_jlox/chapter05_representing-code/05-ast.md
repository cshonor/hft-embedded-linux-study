# 第 5 章 · Representing Code（表示代码 / AST） · 流水线位置

← [本章目录](./README.md) · 上一节：[04-ast.md](./04-ast.md) · 下一节：---

```text
ch4  Token 流
ch5  AST 类型 + Visitor     ← 本章
ch6  Parser：Token → Expr 树
ch7  Interpreter：Visitor 求值
```

**clox**：不在 Java 里建相同 AST 类；编译器内部结构不同，但 **「中间树状表示 → 遍历/生成」** 思路相通（ch17 起）。

---
