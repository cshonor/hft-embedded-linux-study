# 第 15 章 · A Virtual Machine（虚拟机） · 本章定位

← [本章目录](./README.md) · 下一节：[01-an-instruction-execution-machine.md](./01-an-instruction-execution-machine.md)

---

有了 **Chunk**（ch14），本章造 **执行引擎**：**栈式 VM** + **指令指针 ip** + **`run()` 主循环**。先实现 **算术计算器**，验证 bytecode 可跑。

| 对比 | jlox Interpreter | clox VM |
|------|------------------|---------|
| 驱动 | Visitor 递归 AST | **`while` + switch(opcode)** |
| 操作数暂存 | Java 调用栈 / 局部变量 | **显式 Value Stack** |
| 程序计数 | 隐式在递归深度里 | **`ip`** 显式指向当前指令 |

| 小节 | 主题 |
|------|------|
| **§15.1** | **`VM`** · **`ip`** · **`run()`** · switch dispatch |
| **§15.2** | **值栈 (Value Stack)** |
| **§15.3** | 算术 · **`OP_NEGATE`** · **`OP_ADD`** 等 |

---
