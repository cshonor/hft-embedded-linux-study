# 第 21 章 · Global Variables（全局变量）

> **Crafting Interpreters** · [Part III · clox](../README.md) · [本书目录](../../本书目录.md)  
> 在线：[craftinginterpreters.com](https://craftinginterpreters.com/global-variables.html) · [中文在线](https://craftinginterpreters.com/global-variables.html)

## 状态

- [x] 已读（笔记整理）

---

## 一句话

ch20 哈希表 落地为 `vm.globals` → Lox 终于有了 状态。同时引入 语句、`print`、`var`，以及 Pratt 中的 赋值优先级 / `canAssign`。

---

## 专项笔记（一小节一文件）

| 小节 | 主题 | 阅读 |
|------|------|------|
| — | 本章定位 | [00-overview.md](./00-overview.md) |
| ·2 | 语句与状态效果（Statements） | [01-statements.md](./01-statements.md) |
| ·3 | 全局变量声明（Variable Declarations） | [02-variable-declarations.md](./02-variable-declarations.md) |
| ·4 | 读取和赋值（Reading and Assignment） | [03-reading-and-assignment.md](./03-reading-and-assignment.md) |
| ·5 | 赋值的优先级（Assignment Precedence） | [04-assignment-precedence.md](./04-assignment-precedence.md) |
| ·6 | 全局变量管线小结 | [05-global-variables.md](./05-global-variables.md) |
| — | 速记 · 自测 · 进度 |

---

## 逻辑脉络


---

## 速记

## 本章速记

```text
语句    表达式 stmt 末尾 OP_POP · print → OP_PRINT
声明    OP_DEFINE_GLOBAL · 名入常量池 intern
读写    OP_GET/SET_GLOBAL · vm.globals 表
赋值    canAssign 控制 · 仅 ASSIGNMENT 级可 =
错误    未定义全局 · runtimeError + line
```

---

---

## 读后下一步

| 章 | 目录 | 内容 |
|:--:|------|------|
| **22** | [chapter22 · Local Variables](../chapter22_local-variables/) | 栈 slot · **`OP_GET/SET_LOCAL`** |
| **23** | Jumping Back and Forth | 控制流 bytecode |
| **11** jlox | Resolver | distance · 闭包静态绑定（clox ch22+ 对应） |

---

---

## 自测

1. 为什么 `SET_GLOBAL` 执行后还要把值 push 回栈？
2. `canAssign=false` 时遇到 `foo = 1` 会怎样？
3. 全局变量名为何放在常量池而不是指令里嵌字符串？

---

---

## 阅读进度

- [x] 语句 / 全局 var / GET·SET / canAssign 结构梳理（本章笔记）
- [ ] 对照 jlox ch8 Environment 与 clox globals 表
- [ ] 本章 Challenges

