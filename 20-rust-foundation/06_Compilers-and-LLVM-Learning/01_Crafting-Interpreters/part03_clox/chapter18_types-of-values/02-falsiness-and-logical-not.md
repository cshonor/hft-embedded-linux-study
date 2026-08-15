# 第 18 章 · Types of Values（值的类型） · 真假值与逻辑非（Falsiness and Logical Not）

← [本章目录](./README.md) · 上一节：[01-tagged-unions.md](./01-tagged-unions.md) · 下一节：[03-equality-and-comparison.md](./03-equality-and-comparison.md)

---

**Lox 规则**（与 [ch3 规格](../../part01_welcome/chapter03_the-lox-language/README.md) 一致）：

| 值 | Truthy? |
|----|:-------:|
| **`nil`** | 假 |
| **`false`** | 假 |
| 其他（含 **`0`**、**`""`** 将来） | **真** |

**编译 / 指令**：

- 字面量 **`true`/`false`/`nil`** → **`emitConstant(BOOL_VAL(...))`** 等。
- **`!expr`**：compile `expr` → **`OP_NOT`**（VM 按 falsiness 压 `true/false`）。

---
