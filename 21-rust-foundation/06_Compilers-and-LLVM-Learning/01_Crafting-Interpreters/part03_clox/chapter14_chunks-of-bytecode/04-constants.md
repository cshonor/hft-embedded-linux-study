# 第 14 章 · Chunks of Bytecode（字节码块） · §14.5 常量（Constants）

← [本章目录](./README.md) · 上一节：[03-disassembling-chunks.md](./03-disassembling-chunks.md) · 下一节：[05-line-information.md](./05-line-information.md)

---

字面量 **不嵌入** 指令流（避免变长指令、对齐麻烦）→ **常量池 (Constant Pool)**。

| 结构 | 说明 |
|------|------|
| **`constants[]`** | 并行动态数组，存 **`Value`**（早期多为 **double**） |
| **`OP_CONSTANT`** | 后跟 **1 字节索引**（0～255），从池中取常量压栈 |

```text
code:     OP_CONSTANT  0
constants[0] = 1.2
```

| 设计 | 原因 |
|------|------|
| 索引单字节 | 与 clox 后续「单操作数字节码」风格一致；常量过多时再扩展 |
| 池与 code 分离 | 指令长度固定、解码简单 |

**对照**：JVM `ldc`、LLVM **constant pool** 思想相同。

---
