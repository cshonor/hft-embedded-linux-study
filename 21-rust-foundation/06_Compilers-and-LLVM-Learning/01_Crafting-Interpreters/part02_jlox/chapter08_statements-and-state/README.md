# 第 8 章 · Statements and State（语句与状态）

> **Crafting Interpreters** · [Part II · jlox](../README.md) · [本书目录](../../本书目录.md)  
> 在线：[craftinginterpreters.com](https://craftinginterpreters.com/statements-and-state.html) · [中文在线](https://craftinginterpreters.com/statements-and-state.html)

## 状态

- [x] 已读（笔记整理）

---

## 一句话

ch7 像计算器（单次表达式）；ch8 引入 语句 + 变量 → 解释器有记忆（状态）。

---

## 专项笔记（一小节一文件）

| 小节 | 主题 | 阅读 |
|------|------|------|
| — | 本章定位 | [00-overview.md](./00-overview.md) |
| §8.1 | 语句（Statements） | [01-statements.md](./01-statements.md) |
| §8.2 | 变量声明（Variable Declarations） | [02-variable-declarations.md](./02-variable-declarations.md) |
| §8.3 | 环境（Environments） | [03-environments.md](./03-environments.md) |
| §8.4 | 赋值（Assignment） | [04-assignment.md](./04-assignment.md) |
| §8.5 | 块作用域（Block Scope） | [05-block-scope.md](./05-block-scope.md) |
| — | 速记 · 自测 · 进度 |

---

## 逻辑脉络


---

## 速记

## 本章速记

```text
§8.1  Stmt · expr; · print
§8.2  var · 默认 nil
§8.3  Environment + HashMap
§8.4  Assign · 先 parse 再验 l-value
§8.5  块 · 环境链 · shadowing
```

---

---

## 读后下一步

| 章 | 目录 | 内容 |
|:--:|------|------|
| **9** | [chapter09 · Control Flow](../chapter09_control-flow/) | `if` / `while` / `for` · 图灵完备 |

---

---

## 阅读进度

- [x] §8.1～§8.5 结构梳理（本章笔记）
- [ ] 实现 `Stmt` visitor + 嵌套块 shadowing 测试
- [ ] 本章 Challenges

