# 第 14 章 · Chunks of Bytecode（字节码块） · §14.1 什么是字节码（What is Bytecode?）

← [本章目录](./README.md) · 上一节：[00-overview.md](./00-overview.md) · 下一节：[02-chunks-of-instructions.md](./02-chunks-of-instructions.md)

---

三种执行层次（对照 [ch2 编译之山](../../part01_welcome/chapter02_map-of-the-territory/README.md)）：

| 方式 | 优点 | 缺点 |
|------|------|------|
| **AST 解释** | 实现直观、易调试 | 节点分散、缓存差、间接跳转多 |
| **原生机器码** | 最快 | 平台相关、编译复杂 |
| **Bytecode** | **紧凑连续**、缓存友好、**可移植** | 需 VM 或 JIT |

**Bytecode 定位**：介于源码 AST 与 CPU 机器码之间的 **中间表示（IR）**——类似 JVM / Python bytecode / Lua VM 指令。

```text
Source → [Compiler 前端] → Bytecode → VM interpret (或 JIT → 机器码)
```

**本仓库延伸**：[04_Learn-LLVM-17](../../../04_Learn-LLVM-17/) · LLVM IR 是更靠后的 IR 层。

---
