# 第 7 章 · Evaluating Expressions（求值表达式）

> **Crafting Interpreters** · [Part II · jlox](../README.md) · [本书目录](../../本书目录.md)  
> 在线：[craftinginterpreters.com](https://craftinginterpreters.com/evaluating-expressions.html) · [中文在线](https://craftinginterpreters.com/evaluating-expressions.html)

## 状态

- [x] 已读（笔记整理）

---

## 一句话

解释器「睁开眼睛」：从静态 AST → 真正执行并算出结果。

---

## 专项笔记（一小节一文件）

| 小节 | 主题 | 阅读 |
|------|------|------|
| — | 本章定位 | [00-overview.md](./00-overview.md) |
| §7.1 | 表示值（Representing Values） | [01-representing-values.md](./01-representing-values.md) |
| §7.2 | 求值表达式（Evaluating Expressions） | [02-evaluating-expressions.md](./02-evaluating-expressions.md) |
| §7.3 | 运行时错误（Runtime Errors） | [03-runtime-errors.md](./03-runtime-errors.md) |
| §7.4 | 接入解释器（Hooking Up the Interpreter） | [04-hooking-up-the-interpreter.md](./04-hooking-up-the-interpreter.md) |
| — | 速记 · 自测 · 进度 |

---

## 逻辑脉络


---

## 速记

## 本章速记

```text
§7.1  Object 存 Lox 值 · nil→null · Double/Boolean/String
§7.2  Visitor 后序求值 · truthy · 二元左右顺序
§7.3  RuntimeError + 行号 · 不崩 JVM
§7.4  interpret · stringify · REPL 捕获
```

---

---

## 读后下一步

| 章 | 目录 | 内容 |
|:--:|------|------|
| **8** | [chapter08 · Statements and State](../chapter08_statements-and-state/) | **Stmt** · `var` · **Environment** |

---

---

## 阅读进度

- [x] §7.1～§7.4 结构梳理（本章笔记）
- [ ] 跟书实现 `Interpreter` + REPL 算式
- [ ] 本章 Challenges

