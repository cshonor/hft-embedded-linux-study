# 第 7 章 · Evaluating Expressions（求值表达式） · §7.2 求值表达式（Evaluating Expressions）

← [本章目录](./README.md) · 上一节：[01-representing-values.md](./01-representing-values.md) · 下一节：[03-runtime-errors.md](./03-runtime-errors.md)

---

复用 ch5 **Visitor**：`Interpreter implements Expr.Visitor<Object>`。

### 遍历方式：后序（post-order）

- 先求**子表达式**，再应用**当前节点**运算符。
- 与 AST 结构一致：Binary 先 `left`、`right`，再算 `op`。

### 各节点要点

| 节点 | 行为 |
|------|------|
| **Literal** | 原样返回字面量值 |
| **Grouping** | 递归求内部表达式 |
| **Unary** | `-` 数值取负；`!` 用 **truthy** 规则 |
| **Binary** | 算术 / 比较；**左操作数先求值再右**（求值顺序） |

### Truthy（真假值）

- 动态语言常见：并非只有 `true/false` 参与逻辑。
- `!` 等对操作数做 **truthy** 判定（ch9 `and/or` 会延伸）。

**对照 ch3**：§3.4 表达式 · §3.2 动态类型在运行时才落地。

---
