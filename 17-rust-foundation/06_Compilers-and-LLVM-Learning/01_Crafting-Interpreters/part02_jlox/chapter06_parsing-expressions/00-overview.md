# 第 6 章 · Parsing Expressions（解析表达式） · 本章定位

← [本章目录](./README.md) · 下一节：[01-ambiguity-and-the-parsing-game.md](./01-ambiguity-and-the-parsing-game.md)

---

**Part II 第一个重要里程碑**：实现 **Parser**，把 **Token 流** → **AST**（反映语法嵌套）。

| 上一章 | 本章 | 下一章 |
|--------|------|--------|
| ch5 定义 AST + Visitor | **ch6 构造 Expr 树** | ch7 **运行**树（求值） |

| 小节 | 主题 |
|------|------|
| **§6.1** | 歧义 · 优先级 · 结合性 · 分层语法 |
| **§6.2** | 递归下降 · 规则→方法 · `while` 建 Binary |
| **§6.3** | 恐慌模式 · 同步恢复 |
| **§6.4** | `parse()` 接入解释器 |

---
