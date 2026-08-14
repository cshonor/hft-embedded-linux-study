# 第 18 章 · Types of Values（值的类型） · 相等与比较（Equality and Comparison）

← [本章目录](./README.md) · 上一节：[02-falsiness-and-logical-not.md](./02-falsiness-and-logical-not.md) · 下一节：[04-runtime-errors.md](./04-runtime-errors.md)

---

新增 opcode（示意）：

| 源码 | 字节码思路 |
|------|------------|
| **`==` / `!=`** | **`OP_EQUAL`** · **`OP_NOT`**（不等） |
| **`<` / `>`** | **`OP_GREATER`** 等（约定操作数顺序） |
| **`<=` / `>=`** | **编译期脱糖**，减 opcode 数量 |

**`a <= b` 脱糖**：

```text
compile a
compile b
OP_GREATER      // 等价 a > b
OP_NOT          // !(a > b) 即 a <= b
```

| 优点 | 说明 |
|------|------|
| VM 更小 | 少实现 **`OP_LESS_EQUAL`** 等 |
| 经典技巧 | 与 ch9 **`for` 脱糖** 同属「前端改写」 |

**比较类型**：Lox 允许 **`==`** 跨类型；**`<`** 等要求 **number**（运行时检查）。

---
