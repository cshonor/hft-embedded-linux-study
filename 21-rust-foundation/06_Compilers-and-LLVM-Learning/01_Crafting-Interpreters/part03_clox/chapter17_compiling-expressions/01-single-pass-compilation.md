# 第 17 章 · Compiling Expressions（编译表达式） · 单遍编译（Single-Pass Compilation）

← [本章目录](./README.md) · 上一节：[00-overview.md](./00-overview.md) · 下一节：[02-a-pratt-parser.md](./02-a-pratt-parser.md)

---

```text
Source → Scanner (按需 Token)
           ↓
      Compiler: Pratt parse ──emit──► Chunk (bytecode)
           ↓
      VM.run()  (ch15)
```

| 特点 | 说明 |
|------|------|
| **不分配 AST 节点** | 省内存、少指针追逐 |
| **编译即解析** | 每识别一个语法结构，立刻 **`emitByte`** |
| **对照 ch2** | 传统编译器前端 → **IR（此处为 bytecode）** 单趟 |

**jlox 对照**：Parser 产出 `Expr` 树 → Interpreter Visitor；clox **跳过树**，指令流即程序。

---
