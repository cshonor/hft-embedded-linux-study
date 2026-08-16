# 第 21 章 · Global Variables（全局变量） · 本章定位

← [本章目录](./README.md) · 下一节：[01-statements.md](./01-statements.md)

---

**ch20 哈希表** 落地为 **`vm.globals`** → Lox 终于有了 **状态**。同时引入 **语句**、**`print`**、**`var`**，以及 Pratt 中的 **赋值优先级 / `canAssign`**。

| 对比 | jlox ch8 | clox ch21 |
|------|----------|-----------|
| 存储 | **`Environment` HashMap** | **`Table vm.globals`** |
| 声明 | **`Stmt.Var` + define** | **`OP_DEFINE_GLOBAL`** |
| 读取/写 | **`get` / `assign` + distance** | **`OP_GET/SET_GLOBAL`**（暂仅全局） |
| 表达式语句 | Visitor | compile 后 **`OP_POP`** |

| 主题 | 要点 |
|------|------|
| **Statements** | 栈效应为 0 · **`OP_POP`** |
| **`var` 声明** | **`OP_DEFINE_GLOBAL`** |
| **读/写** | **`OP_GET/SET_GLOBAL`** · 未定义报错 |
| **Assignment** | **`canAssign`** · 非法 l-value |

---
