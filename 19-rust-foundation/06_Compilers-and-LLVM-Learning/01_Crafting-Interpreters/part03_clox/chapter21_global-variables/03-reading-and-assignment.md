# 第 21 章 · Global Variables（全局变量） · 读取和赋值（Reading and Assignment）

← [本章目录](./README.md) · 上一节：[02-variable-declarations.md](./02-variable-declarations.md) · 下一节：[04-assignment-precedence.md](./04-assignment-precedence.md)

---

**标识符作为表达式**（读取）：

```text
OP_GET_GLOBAL  name_index
→ tableGet(&vm.globals, name) → push(value)
  失败 → runtimeError("Undefined variable '...'.")
```

**赋值 `name = expr`**：

```text
compile expr          // 先算右值
OP_SET_GLOBAL  name   // pop 写入表 · 再 push（表达式值=赋值结果）
```

| Opcode | 作用 |
|--------|------|
| **`OP_DEFINE_GLOBAL`** | 声明：pop 初值 · **新建** 绑定 |
| **`OP_GET_GLOBAL`** | 读：push 值 |
| **`OP_SET_GLOBAL`** | 写：pop 赋值 · push 同一值 |

**常量池存名字**：每条指令只带 **1 字节索引** → 名字字符串 **intern 一次**。

---
