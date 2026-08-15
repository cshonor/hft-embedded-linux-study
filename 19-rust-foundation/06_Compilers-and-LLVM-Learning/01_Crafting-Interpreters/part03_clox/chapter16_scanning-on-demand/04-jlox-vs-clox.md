# 第 16 章 · Scanning on Demand（按需扫描） · jlox vs clox 扫描器架构

← [本章目录](./README.md) · 上一节：[03-identifiers-and-keywords.md](./03-identifiers-and-keywords.md) · 下一节：---

```text
jlox:
  source ──► Scanner.scanTokens() ──► List<Token> ──► Parser

clox:
  source ──► scanToken() ◄── Compiler.advance()（按需）
                │
                └──► 直接 emit bytecode（ch17+）
```

---
