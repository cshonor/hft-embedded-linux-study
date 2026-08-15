# 第 15 章 · A Virtual Machine（虚拟机）

> **Crafting Interpreters** · [Part III · clox](../README.md) · [本书目录](../../本书目录.md)  
> 在线：[craftinginterpreters.com](https://craftinginterpreters.com/a-virtual-machine.html) · [中文在线](https://craftinginterpreters.com/a-virtual-machine.html)

## 状态

- [x] 已读（笔记整理）

---

## 一句话

有了 Chunk（ch14），本章造 执行引擎：栈式 VM + 指令指针 ip + `run()` 主循环。先实现 算术计算器，验证 bytecode 可跑。

---

## 专项笔记（一小节一文件）

| 小节 | 主题 | 阅读 |
|------|------|------|
| — | 本章定位 | [00-overview.md](./00-overview.md) |
| §15.1 | 指令执行机（An Instruction Execution Machine） | [01-an-instruction-execution-machine.md](./01-an-instruction-execution-machine.md) |
| §15.2 | 值栈操作（A Value Stack Manipulator） | [02-a-value-stack-manipulator.md](./02-a-value-stack-manipulator.md) |
| §15.3 | 算术计算器（An Arithmetic Calculator） | [03-an-arithmetic-calculator.md](./03-an-arithmetic-calculator.md) |
| ·5 | VM 与 Chunk 协作 | [04-vm-chunk.md](./04-vm-chunk.md) |
| — | 速记 · 自测 · 进度 |

---

## 逻辑脉络


---

## 速记

## 本章速记

```text
§15.1  VM · ip · run() · switch 分发
§15.2  Value Stack · push/pop · STACK_MAX
§15.3  OP_CONSTANT/NEGATE/ADD/.../RETURN · 栈式算术
```

---

---

## 读后下一步

| 章 | 目录 | 内容 |
|:--:|------|------|
| **16** | [chapter16 · Scanning](../chapter16_scanning-on-demand/) | **按需** Token · 关键字 DFA |
| **17** | Compiling Expressions | 源码 → **emit Chunk** |
| **18** | Types of Values | **NaN tagging** 统一 Value |

---

---

## 自测

1. `ip` 在读取 `OP_CONSTANT` 的操作数（索引字节）时如何推进？
2. 栈式 VM 执行 `a + b * c` 时，编译器应保证什么顺序？
3. clox 的 `run()` 与 jlox 的 `visitBinaryExpr` 在控制流上有何本质不同？

---

---

## 阅读进度

- [x] §15.1～§15.3 结构梳理（本章笔记）
- [ ] 手写一段 bytecode 用 VM 跑通四则运算
- [ ] 本章 Challenges

