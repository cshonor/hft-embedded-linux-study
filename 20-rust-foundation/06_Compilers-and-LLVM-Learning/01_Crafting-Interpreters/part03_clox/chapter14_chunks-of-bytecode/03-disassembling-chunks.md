# 第 14 章 · Chunks of Bytecode（字节码块） · §14.4 反汇编块（Disassembling Chunks）

← [本章目录](./README.md) · 上一节：[02-chunks-of-instructions.md](./02-chunks-of-instructions.md) · 下一节：[04-constants.md](./04-constants.md)

---

VM 是「黑盒」→ 需要 **`disassembleChunk()`** 把二进制打印成可读指令。

```text
0000  1 OP_CONSTANT    0 '1.2'
0002     OP_RETURN
```

| 用途 | 说明 |
|------|------|
| 开发调试 | 对照源码与生成字节码 |
| 单测 | 断言编译输出 |
| 对照 jlox | 无 AST `toString()`，靠 disassembler |

**`disassembleInstruction(chunk, offset)`**：读 opcode → switch → 打印操作数（如常量索引）。

---
