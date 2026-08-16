# 第 17 章 · Compiling Expressions（编译表达式） · 发出字节码（Emitting Bytecode）

← [本章目录](./README.md) · 上一节：[02-a-pratt-parser.md](./02-a-pratt-parser.md) · 下一节：[04-handling-syntax-errors.md](./04-handling-syntax-errors.md)

---

**编译器辅助**：

| API | 作用 |
|-----|------|
| **`emitByte(op)`** | 写 opcode（+ 行号） |
| **`emitConstant(value)`** | 常量入池 → **`OP_CONSTANT` idx** |
| **`emitReturn()`** | 结束片段 |

**典型 lowering**（栈 VM 顺序）：

| 源码 | 编译策略 |
|------|----------|
| 数字 `1.2` | `emitConstant(1.2)` |
| `(expr)` | 编译 `expr`（括号不 emit） |
| **`-x`** | 先 **`compile x`**（push）→ **`OP_NEGATE`** |
| **`a + b`** | compile `a` → compile `b` → **`OP_ADD`** |

**关键**：一元 `-` **先子后父**——操作数先入栈，再 negate，与 ch15 栈式语义一致。

**分组 `(`**：prefix 函数 **`(`** 内调 `expression()`，再 **`consume(RIGHT_PAREN)`**。

---
