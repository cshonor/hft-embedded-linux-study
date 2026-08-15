# 第 6 章 · Parsing Expressions（解析表达式） · 流水线里程碑

← [本章目录](./README.md) · 上一节：[04-wiring-up-the-parser.md](./04-wiring-up-the-parser.md) · 下一节：---

```text
ch4  Scanning      Token[]
ch5  AST 类型
ch6  Parsing       Expr 树     ← 本章：一维 → 结构化
ch7  Evaluating    运行树
ch8+ 语句、作用域、控制流…
```

**clox**：不共用 jlox 的 `Parser` 类；**ch17** 用 **Pratt 解析器** + 编译到字节码（另一种手写 parser 风格）。

---
