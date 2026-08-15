# 第 9 章 · Control Flow（控制流）

> **Crafting Interpreters** · [Part II · jlox](../README.md) · [本书目录](../../本书目录.md)  
> 在线：[craftinginterpreters.com](https://craftinginterpreters.com/control-flow.html) · [中文在线](https://craftinginterpreters.com/control-flow.html)

## 状态

- [x] 已读（笔记整理）

---

## 一句话

加入 条件分支 + 循环 → Lox 解释器在此达到 图灵完备（可表达通用计算）。

---

## 专项笔记（一小节一文件）

| 小节 | 主题 | 阅读 |
|------|------|------|
| — | 本章定位 | [00-overview.md](./00-overview.md) |
| §9.2 | 条件执行（If Statements） | [01-if-statements.md](./01-if-statements.md) |
| §9.3 | 逻辑操作符（Logical Operators） | [02-logical-operators.md](./02-logical-operators.md) |
| §9.4 | While 循环（While Loops） | [03-while-loops.md](./03-while-loops.md) |
| §9.5 | For 循环（For Loops） | [04-for-loops.md](./04-for-loops.md) |
| ·6 | 三阶段能力小结（ch7～9） | [05-ch7-9.md](./05-ch7-9.md) |
| — | 速记 · 自测 · 进度 |

---

## 逻辑脉络


---

## 速记

## 本章速记

```text
§9.2  if/else · dangling else · 贪婪绑定
§9.3  and/or 短路 · 返回操作数值非必 bool
§9.4  while · Stmt.While
§9.5  for 脱糖为 block + while
```

---

---

## 读后下一步

| 章 | 目录 | 内容 |
|:--:|------|------|
| **10** | [chapter10 · Functions](../chapter10_functions/) | **`fun`** · 一等函数 |
| **11** | Resolving and Binding | 闭包 · **Resolver** |

---

---

## 阅读进度

- [x] §9.2～§9.5 结构梳理（本章笔记）
- [ ] 测试悬挂 else、短路、`for` 脱糖后的 AST
- [ ] 本章 Challenges

