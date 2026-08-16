# 第 17 章 · Compiling Expressions（编译表达式）

> **Crafting Interpreters** · [Part III · clox](../README.md) · [本书目录](../../本书目录.md)  
> 在线：[craftinginterpreters.com](https://craftinginterpreters.com/compiling-expressions.html) · [中文在线](https://craftinginterpreters.com/compiling-expressions.html)

## 状态

- [x] 已读（笔记整理）

---

## 一句话

前端扫描器（ch16）与后端 VM（ch15）正式接合：编写 字节码编译器，把源码 直接编译成 Chunk，不建 AST。

---

## 专项笔记（一小节一文件）

| 小节 | 主题 | 阅读 |
|------|------|------|
| — | 本章定位 | [00-overview.md](./00-overview.md) |
| ·2 | 单遍编译（Single-Pass Compilation） | [01-single-pass-compilation.md](./01-single-pass-compilation.md) |
| ·3 | 普拉特解析器（A Pratt Parser） | [02-a-pratt-parser.md](./02-a-pratt-parser.md) |
| ·4 | 发出字节码（Emitting Bytecode） | [03-emitting-bytecode.md](./03-emitting-bytecode.md) |
| ·5 | 语法错误处理（Handling Syntax Errors） | [04-handling-syntax-errors.md](./04-handling-syntax-errors.md) |
| ·6 | 编译管线（本章结束时） | [05-compiling-expressions.md](./05-compiling-expressions.md) |
| — | 速记 · 自测 · 进度 |

---

## 逻辑脉络


---

## 速记

## 本章速记

```text
单遍    无 AST · parse 同时 emit Chunk
Pratt   rules[] · prefix/infix · precedence
Emit    先 compile 子表达式再 OP_* · 一元 − 后 NEGATE
错误    panicMode + synchronize 到语句边界
```

---

---

## 读后下一步

| 章 | 目录 | 内容 |
|:--:|------|------|
| **18** | [chapter18 · Types of Values](../chapter18_types-of-values/) | **`Value`**  tagged union · bool/nil |
| **19** | Strings | **`ObjString`** · 堆对象 |
| **21** | Global Variables | **`canAssign`** · 全局变量 opcode |

---

---

## 自测

1. Pratt 与 jlox 递归下降相比，换运算符优先级时改哪里？
2. 为什么 `-123` 的编译顺序是 `CONSTANT 123` 然后 `OP_NEGATE`？
3. `panicMode` 存在的意义是什么——若没有会怎样？

---

---

## 阅读进度

- [x] 单遍 / Pratt / Emit / 错误恢复 结构梳理（本章笔记）
- [ ] 对照 `rules[]` 列出各 Token 的 prefix/infix
- [ ] 本章 Challenges

