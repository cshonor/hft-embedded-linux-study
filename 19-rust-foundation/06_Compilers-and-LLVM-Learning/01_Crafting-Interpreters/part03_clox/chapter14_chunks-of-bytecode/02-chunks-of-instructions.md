# 第 14 章 · Chunks of Bytecode（字节码块） · §14.3 指令块（Chunks of Instructions）

← [本章目录](./README.md) · 上一节：[01-what-is-bytecode.md](./01-what-is-bytecode.md) · 下一节：[03-disassembling-chunks.md](./03-disassembling-chunks.md)

---

**`Chunk`** 核心结构（C）：

| 字段 | 作用 |
|------|------|
| **`code[]`** | 动态扩展的 **uint8_t** 数组，存 **Opcode + 操作数** |
| **`count` / `capacity`** | 已用长度 / 容量 |

**写入 API**（典型）：

- **`writeChunk(chunk, byte, line)`** — 追加一字节，同步记录行号。
- **`emitByte`** / **`emitReturn`** — 编译器侧封装。

**Opcode 枚举**（随章节增长）：

```c
typedef enum {
  OP_CONSTANT,
  OP_ADD,
  OP_SUBTRACT,
  OP_MULTIPLY,
  OP_DIVIDE,
  OP_NEGATE,
  OP_RETURN,
} OpCode;
```

早期 Chunk 只支撑 **算术计算器**；后续章节不断 **扩展 opcode 表**。

---
