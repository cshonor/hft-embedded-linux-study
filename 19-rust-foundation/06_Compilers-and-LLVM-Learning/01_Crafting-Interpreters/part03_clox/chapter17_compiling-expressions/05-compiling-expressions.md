# 第 17 章 · Compiling Expressions（编译表达式） · 编译管线（本章结束时）

← [本章目录](./README.md) · 上一节：[04-handling-syntax-errors.md](./04-handling-syntax-errors.md) · 下一节：---

```text
compile(source):
  initScanner(source)
  // Pratt + emit
  emitReturn()
  if (!hadError) interpret(&chunk)
```

---
