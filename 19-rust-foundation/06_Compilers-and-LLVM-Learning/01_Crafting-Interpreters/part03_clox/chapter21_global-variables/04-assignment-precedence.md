# 第 21 章 · Global Variables（全局变量） · 赋值的优先级（Assignment Precedence）

← [本章目录](./README.md) · 上一节：[03-reading-and-assignment.md](./03-reading-and-assignment.md) · 下一节：[05-ast.md](./05-ast.md)

---

**`=` 是表达式**，但左值必须是 **可赋值目标**（变量；后续 field 等）。

**非法**：`a + b = c` · `1 = 2`

**Pratt 机制：`canAssign` 标志**

| 传递 | 含义 |
|------|------|
| **`parsePrecedence(PREC_ASSIGNMENT)`** | 顶层允许赋值 |
| 更高优先级 infix（如 **`+`**）调用子 parse 时传 **`canAssign = false`** |
| **`variable()` prefix** | 读变量；若 **`canAssign && match(EQUAL)`** → 生成 **SET** 而非 GET |

```text
parsePrecedence(ASSIGNMENT):
  canAssign = true  →  identifier 后可吃 '='

parsePrecedence(ADDITION):  // 在 + 的 infix 里
  compile left
  compile right with canAssign=false
  → 右侧不会出现非法 a = ...
```

**对照 jlox ch8**：Parser 先 parse 表达式再 **回溯** 成 `Assign`；clox **编译期** 用 **`canAssign`** 约束，**无 AST 回溯**。

---
