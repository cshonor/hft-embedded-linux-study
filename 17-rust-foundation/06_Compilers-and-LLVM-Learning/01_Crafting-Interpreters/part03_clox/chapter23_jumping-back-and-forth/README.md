# 第 23 章 · Jumping Back and Forth（来回跳转）

> **Crafting Interpreters** · [Part III · clox](../README.md) · [本书目录](../../本书目录.md)  
> 在线：[craftinginterpreters.com](https://craftinginterpreters.com/jumping-back-and-forth.html) · [中文在线](https://craftinginterpreters.com/jumping-back-and-forth.html)

## 状态

- [x] 已读（笔记整理）

---

## 一句话

字节码是 一维扁平指令流 → 控制流 = 改 `ip`（跳转）。本章实现 `if` / `and`·`or` / `while` / `for`，并引入经典 回填 (Backpatching)。

---

## 专项笔记（一小节一文件）

| 小节 | 主题 | 阅读 |
|------|------|------|
| — | 本章定位 | [00-overview.md](./00-overview.md) |
| §23.1 | If 语句（If Statements） | [01-if-statements.md](./01-if-statements.md) |
| §23.2 | 逻辑运算符（Logical Operators） | [02-logical-operators.md](./02-logical-operators.md) |
| §23.3 | While 循环（While Statements） | [03-while-statements.md](./03-while-statements.md) |
| §23.4 | For 循环（For Statements） | [04-for-statements.md](./04-for-statements.md) |
| ·6 | 跳转指令族（小结） | [05-jumping-back-and-forth.md](./05-jumping-back-and-forth.md) |
| — | 速记 · 自测 · 进度 |

---

## 逻辑脉络


---

## 速记

## 本章速记

```text
§23.1  if · JUMP_IF_FALSE/JUMP · backpatch 占位再修补
§23.2  and/or 短路 · 栈顶留左值 · 复用条件跳转
§23.3  while · OP_LOOP 回跳
§23.4  for 脱糖为 init+while+increment
```

---

---

## 读后下一步

| 章 | 目录 | 内容 |
|:--:|------|------|
| **24** | [chapter24 · Calls](../chapter24_calling-and-closures/) | **CallFrame** · **`OP_CALL`** · return |
| **25** | Objects | 堆对象扩展 |

---

---

## 自测

1. 为什么回填偏移用「字节」而不是「指令序号」？
2. `a or b` 在左 truthy 时栈上最终剩几个值？
3. `for` 脱糖后，`continue`（若实现）应改哪几个 jump 目标？

---

---

## 阅读进度

- [x] §23.1～§23.4 结构梳理（本章笔记）
- [ ] 画一条 `if/else` 的 bytecode 布局与 patch 点
- [ ] 本章 Challenges

