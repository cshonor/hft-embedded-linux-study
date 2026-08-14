# 第 6 章 · Parsing Expressions（解析表达式）

> **Crafting Interpreters** · [Part II · jlox](../README.md) · [本书目录](../../本书目录.md)  
> 在线：[craftinginterpreters.com](https://craftinginterpreters.com/parsing-expressions.html) · [中文在线](https://craftinginterpreters.com/parsing-expressions.html)

## 状态

- [x] 已读（笔记整理）

---

## 一句话

Part II 第一个重要里程碑：实现 Parser，把 Token 流 → AST（反映语法嵌套）。

---

## 专项笔记（一小节一文件）

| 小节 | 主题 | 阅读 |
|------|------|------|
| — | 本章定位 | [00-overview.md](./00-overview.md) |
| §6.1 | 歧义与解析游戏（Ambiguity and the Parsing Game） | [01-ambiguity-and-the-parsing-game.md](./01-ambiguity-and-the-parsing-game.md) |
| §6.2 | 递归下降解析（Recursive Descent Parsing） | [02-recursive-descent-parsing.md](./02-recursive-descent-parsing.md) |
| §6.3 | 语法错误（Syntax Errors） | [03-syntax-errors.md](./03-syntax-errors.md) |
| §6.4 | 组合起来（Wiring up the Parser） | [04-wiring-up-the-parser.md](./04-wiring-up-the-parser.md) |
| ·6 | 流水线里程碑 | [05-parsing-expressions.md](./05-parsing-expressions.md) |
| — | 速记 · 自测 · 进度 |

---

## 逻辑脉络


---

## 速记

## 本章速记

```text
§6.1  歧义 · precedence/associativity · 分层 equality→…→primary
§6.2  递归下降 · 一规则一方法 · while 建 Binary 左结合
§6.3  panic mode · synchronize 到 ; 或关键字
§6.4  parse() → Expr | null
```

---

---

## 读后下一步

| 章 | 目录 | 内容 |
|:--:|------|------|
| **7** | [chapter07 · Evaluating Expressions](../chapter07_evaluating-expressions/) | **Visitor 解释器** · AST → 值 |
| **8** | Statements and State | 语句 · 环境 · 副作用 |

ch7 前建议：用 **AstPrinter** 对 REPL 输入验几棵表达式树。

---

---

## 自测 / 对照（可选）

- [ ] 画出 `1 + 2 * 3` 的 AST（哪个运算符更深？）。
- [ ] 说明 `6 / 3 - 1` 在**左结合**下对应哪棵树。
- [ ] 写出 `term()` 里 `while (match(PLUS, MINUS))` 循环在做什么。
- [ ] 若源码 `var x = ;`，同步点会在哪里恢复？（概念上）

---

---

## 阅读进度

- [x] §6.1～§6.4 结构梳理（本章笔记）
- [ ] 跟书实现 `Parser` + `AstPrinter` 联调
- [ ] 本章 Challenges

