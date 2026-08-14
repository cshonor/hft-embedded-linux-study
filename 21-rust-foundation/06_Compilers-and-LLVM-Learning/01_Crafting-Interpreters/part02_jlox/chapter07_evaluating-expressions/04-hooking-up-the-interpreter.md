# 第 7 章 · Evaluating Expressions（求值表达式） · §7.4 接入解释器（Hooking Up the Interpreter）

← [本章目录](./README.md) · 上一节：[03-runtime-errors.md](./03-runtime-errors.md) · 下一节：---

| API | 作用 |
|-----|------|
| **`interpret(Expr)`** | 对外入口：解释一棵表达式 AST |
| **`stringify(Object)`** | Java 值 → Lox 风格**字符串**（供 `print` / REPL 显示） |

- 主程序 / REPL **捕获 `RuntimeError`** → 报错后继续提示符，**不退出**进程。

```text
parse() → Expr
interpret(expr) → Object
stringify(result) → 用户可见输出
```

---
