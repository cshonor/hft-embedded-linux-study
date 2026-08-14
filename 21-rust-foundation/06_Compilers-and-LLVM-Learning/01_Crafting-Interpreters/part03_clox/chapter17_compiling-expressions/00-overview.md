# 第 17 章 · Compiling Expressions（编译表达式） · 本章定位

← [本章目录](./README.md) · 下一节：[01-single-pass-compilation.md](./01-single-pass-compilation.md)

---

**前端扫描器（ch16）与后端 VM（ch15）正式接合**：编写 **字节码编译器**，把源码 **直接编译成 Chunk**，不建 AST。

| 对比 | jlox | clox |
|------|------|------|
| 中间表示 | **AST** | **无 AST** |
| 趟数 | 解析 → 执行（多趟） | **单遍**：边 parse 边 emit |
| 表达式解析 | 递归下降（ch6） | **Pratt 解析器** |

| 主题 | 要点 |
|------|------|
| **单遍编译** | parse + emit 同步 |
| **Pratt Parser** | 前缀/中缀分发表 · 优先级 |
| **Emitting** | 字面量 · 括号 · 一元/二元算术 |
| **错误恢复** | **`panicMode`** · 同步到语句边界 |

---
