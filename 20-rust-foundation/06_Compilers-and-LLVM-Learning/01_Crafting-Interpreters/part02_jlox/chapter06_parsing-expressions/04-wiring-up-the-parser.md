# 第 6 章 · Parsing Expressions（解析表达式） · §6.4 组合起来（Wiring up the Parser）

← [本章目录](./README.md) · 上一节：[03-syntax-errors.md](./03-syntax-errors.md) · 下一节：[05-ast.md](./05-ast.md)

---

- 编写 **`parse()`** 启动解析。
- **早期阶段**：只解析并返回**单个表达式**（语句、声明留 ch8+）。
- 捕获 **`ParseError`** → 返回 **`null`**，避免解释器在坏语法上崩溃或挂起。

```text
Scanner.scan() → List<Token>
Parser.parse() → Expr 或 null
（ch7）Interpreter.interpret(expr) → 值
```

---
