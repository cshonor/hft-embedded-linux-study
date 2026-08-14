# 第 15 章 · A Virtual Machine（虚拟机） · §15.2 值栈操作（A Value Stack Manipulator）

← [本章目录](./README.md) · 上一节：[01-an-instruction-execution-machine.md](./01-an-instruction-execution-machine.md) · 下一节：[03-an-arithmetic-calculator.md](./03-an-arithmetic-calculator.md)

---

表达式求值需要 **暂存中间结果** → **后进先出** 的 **固定大小栈**（数组 + **`stackTop`** 指针）。

| 操作 | 说明 |
|------|------|
| **`push(value)`** | `*stackTop++ = value` |
| **`pop()`** | `return *--stackTop` |
| **`peek(offset)`** | 看栈顶下第 n 个（少弹栈） |

**为何栈式？**

- 二元运算自然顺序：**左操作数、右操作数入栈 → 运算符指令弹出并压回结果**。
- 与 **JVM、Forth、RPN 计算器** 同族。
- 局部「虚拟寄存器」= 栈槽，无需为每个临时量分配命名 slot（早期章节）。

**栈溢出**：固定 **`STACK_MAX`**；压栈前检查（运行时错误）。

**jlox 对照**：Visitor 返回值在 **Java 栈**；clox 把栈 **显式化**，便于后续 **帧指针 / call frame**（ch24）。

---
