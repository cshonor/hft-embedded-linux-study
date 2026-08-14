# 第 3 章 · The Lox Language（Lox 语言概览）

> **Crafting Interpreters** · [Part I · Welcome](../README.md) · [本书目录](../../本书目录.md)  
> 在线：[craftinginterpreters.com](https://craftinginterpreters.com/the-lox-language.html) · [中文在线](https://craftinginterpreters.com/the-lox-language.html)

## 状态

- [x] 已读（笔记整理）

---

## 一句话

Part I 最后一章：在写解释器之前，完整介绍 Lox 的语法与语义——友好、平缓，不立刻陷入实现细节。读完 = 拿到接下来 jlox / clox 要实现的目标蓝图。

---

## 专项笔记（一小节一文件）

| 小节 | 主题 | 阅读 |
|------|------|------|
| — | 本章定位 | [00-overview.md](./00-overview.md) |
| §3.1 | 你好，Lox（Hello, Lox） | [01-hello-lox.md](./01-hello-lox.md) |
| §3.2 | 一种高级语言（A High-Level Language） | [02-a-high-level-language.md](./02-a-high-level-language.md) |
| §3.3 | 数据类型（Data Types） | [03-data-types.md](./03-data-types.md) |
| §3.4 | 表达式（Expressions） | [04-expressions.md](./04-expressions.md) |
| §3.5 | 语句（Statements） | [05-statements.md](./05-statements.md) |
| §3.6 | 变量（Variables） | [06-variables.md](./06-variables.md) |
| §3.7 | 控制流（Control Flow） | [07-control-flow.md](./07-control-flow.md) |
| §3.8 | 函数（Functions） | [08-functions.md](./08-functions.md) |
| §3.9 | 类（Classes） | [09-classes.md](./09-classes.md) |
| §3.10 | 标准库（The Standard Library） | [10-the-standard-library.md](./10-the-standard-library.md) |
| ·12 | Lox 特性 → 本书章节速查 | [11-lox.md](./11-lox.md) |
| — | 速记 · 自测 · 进度 |

---

## 逻辑脉络


---

## 速记

## 本章速记

```text
C 系语法 · 动态类型 · GC
类型：bool / number / string / nil
表达式有值 · 语句有副作用 · and/or 短路
var · if/while/for · fun 一等 + 闭包
class / this / init · 单继承 < · super
标准库只有 clock()
```

---

---

## 读后下一步

Part I 读完 → **Part II 开始写代码**

| 章 | 目录 | 内容 |
|:--:|------|------|
| **4** | [chapter04 · Scanning](../../part02_jlox/chapter04_scanning/) | 流水线第一站：**扫描器** → Token |

建议：写 jlox 前扫一眼 **附录 I** 语法；实现中遇歧义回查本章 + A1。

---

---

## 自测 / 对照（可选）

- [ ] 写 3 行 Lox 代码：含 `var`、闭包、`class` 与 `init` 各一。
- [ ] 解释：`and` 短路为何算「控制流味道」？
- [ ] 对比 Rust：`Lox 动态类型` vs `Rust 静态 + Option` 在**何时**发现类型错误。
- [ ] 说明 Lox **无 `new`** 实例化与 Java/C++ 的差异。

---

---

## 阅读进度

- [x] §3.1～§3.10 结构梳理（本章笔记）
- [ ] Design Note *Expressions and Statements*
- [ ] 附录 I Lox Grammar（写 ch4 前建议）
- [ ] Part I 完成 → 进入 **ch4 Scanning**

