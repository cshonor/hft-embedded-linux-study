# 第 14 章 · Chunks of Bytecode（字节码块）

> **Crafting Interpreters** · [Part III · clox](../README.md) · [本书目录](../../本书目录.md)  
> 在线：[craftinginterpreters.com](https://craftinginterpreters.com/chunks-of-bytecode.html) · [中文在线](https://craftinginterpreters.com/chunks-of-bytecode.html)

## 状态

- [x] 已读（笔记整理）

---

## 一句话

Part III 奠基：在造 VM 之前，先定义 字节码如何存放。clox 用 C 手写 紧凑指令序列 + 常量池 + 行号表，告别 jlox 的 AST 指针树。

---

## 专项笔记（一小节一文件）

| 小节 | 主题 | 阅读 |
|------|------|------|
| — | 本章定位 | [00-overview.md](./00-overview.md) |
| §14.1 | 什么是字节码（What is Bytecode?） | [01-what-is-bytecode.md](./01-what-is-bytecode.md) |
| §14.3 | 指令块（Chunks of Instructions） | [02-chunks-of-instructions.md](./02-chunks-of-instructions.md) |
| §14.4 | 反汇编块（Disassembling Chunks） | [03-disassembling-chunks.md](./03-disassembling-chunks.md) |
| §14.5 | 常量（Constants） | [04-constants.md](./04-constants.md) |
| §14.6 | 行号信息（Line Information） | [05-line-information.md](./05-line-information.md) |
| ·7 | Chunk 内存布局（小结） | [06-chunks-of-bytecode-chunk.md](./06-chunks-of-bytecode-chunk.md) |
| — | 速记 · 自测 · 进度 |

---

## 逻辑脉络


---

## 速记

## 本章速记

```text
§14.1  Bytecode：紧凑、可移植、VM 执行
§14.3  Chunk 动态 code[] · OpCode 枚举
§14.4  disassembleChunk 调试
§14.5  常量池 + OP_CONSTANT 单字节索引
§14.6  lines[] 平行数组报行号
```

---

---

## 读后下一步

| 章 | 目录 | 内容 |
|:--:|------|------|
| **15** | [chapter15 · VM](../chapter15_a-virtual-machine/) | **`ip`** · **`run()`** · 值栈 · 算术 |
| **16** | [chapter16 · Scanning](../chapter16_scanning-on-demand/) | 按需 Token |
| **17** | Compiling Expressions | Token → **编译进 Chunk** |

---

---

## 自测

1. 为什么 `1.2` 不直接编码进 `OP_ADD` 之类的指令里？
2. `lines[]` 为何按**字节**而非按「指令」索引？
3. 反汇编器在 clox 开发流程里相当于 jlox 的什么工具？

---

---

## 阅读进度

- [x] §14.1、§14.3～§14.6 结构梳理（本章笔记）
- [ ] 本地 clox：跑 disassemble 示例 Chunk
- [ ] 本章 Challenges

