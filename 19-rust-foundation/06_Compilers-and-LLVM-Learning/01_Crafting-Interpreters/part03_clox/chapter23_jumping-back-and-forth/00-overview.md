# 第 23 章 · Jumping Back and Forth（来回跳转） · 本章定位

← [本章目录](./README.md) · 下一节：[01-if-statements.md](./01-if-statements.md)

---

字节码是 **一维扁平指令流** → **控制流 = 改 `ip`（跳转）**。本章实现 **`if` / `and`·`or` / `while` / `for`**，并引入经典 **回填 (Backpatching)**。

| 对比 | jlox ch9 | clox ch23 |
|------|----------|-----------|
| `if` | Visitor 递归分支 | **`OP_JUMP_IF_FALSE` + `OP_JUMP`** |
| `while` | Java `while` | **`OP_LOOP` 负偏移** |
| `for` | AST 脱糖 | **编译期脱糖为 while 结构** |
| `and`/`or` | 短路不求值右操作数 | **跳转 + 栈顶保留左值** |

| 小节 | 主题 |
|------|------|
| **§23.1** | **`if`** · 回填 |
| **§23.2** | **`and` / `or`** · 短路 |
| **§23.3** | **`while`** · **`OP_LOOP`** |
| **§23.4** | **`for`** · 脱糖 |

---
