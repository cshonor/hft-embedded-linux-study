# 第 14 章 · Chunks of Bytecode（字节码块） · §14.6 行号信息（Line Information）

← [本章目录](./README.md) · 上一节：[04-constants.md](./04-constants.md) · 下一节：[06-chunks-of-bytecode-chunk.md](./06-chunks-of-bytecode-chunk.md)

---

运行时错误需报 **源码行号** → 与 **`code[]` 平行的 `lines[]`**。

| 规则 | 说明 |
|------|------|
| **逐字节** | `lines[i]` = `code[i]` 对应源码行 |
| 与 `writeChunk(..., line)` 同步 | 每条 emit 传入当前 **Scanner 行** |
| 运行时 | ip 指向出错指令 → 查 **`lines[ip - chunk->code]`** |

**代价**：多一个 int 数组 · **收益**：用户友好的 stack trace（无 AST 节点行号字段时仍可用）。

---
