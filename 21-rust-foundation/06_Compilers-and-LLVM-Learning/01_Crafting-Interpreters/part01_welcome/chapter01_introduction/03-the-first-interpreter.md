# 第 1 章 · Welcome（Introduction） · §1.3 第一个解释器（The First Interpreter）

← [本章目录](./README.md) · 上一节：[02-how-the-book-is-organized.md](./02-how-the-book-is-organized.md) · 下一节：---

### jlox（Part II · Java）

- 第二个 Part 用 **Java** 写第一个解释器 **`jlox`**（树遍历）。
- 选 Java 的原因：
  - 足够**高级**，可专注**语言概念与语义**；
  - 写出**最简单、最干净**的实现；
  - 不被 C 层级的内存管理等细节拖住（GC、OOP 现成）。

```text
jlox 目标：把 Lox「说对了」—— 语义清晰优先，不追求速度
```

### clox（Part III · C）— 紧接 §1.3 后引出

- 第一个解释器**慢**；第三部分用 **C** 写 **`clox`**（字节码 VM）。
- 深入：**内存管理**、**字节码编译**、执行速度、GC 等底层机制。

```text
clox 目标：把 Lox「跑透了」—— 贴近机器、更快
```

**本仓库衔接**：jlox → **03**《自制编译器》前端直觉；clox → RFR 内存/类型 · **04** LLVM ch07 · ch30 Optimization。

---
