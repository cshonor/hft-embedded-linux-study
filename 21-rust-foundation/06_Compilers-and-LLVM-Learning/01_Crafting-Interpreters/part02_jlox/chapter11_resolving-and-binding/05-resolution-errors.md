# 第 11 章 · Resolving and Binding（解析与绑定） · §11.5 解析错误（Resolution Errors）

← [本章目录](./README.md) · 上一节：[04-interpreting-resolved-variables.md](./04-interpreting-resolved-variables.md) · 下一节：[06-resolver-interpreter.md](./06-resolver-interpreter.md)

---

静态扫描可做的 **compile-time（解析期）检查**：

| 规则 | 示例 |
|------|------|
| **`return` 只能在函数内** | 顶层 `return 1;` → **Resolver 报错** |
| 重复参数名 | `fun f(a, a)` |
| （后续 ch12）**`this` 只能在方法内** | 类外 `this` |

与 ch4 **语法错误** 区分：AST 合法，但 **语义不合法**。

**管线**：

```text
Lox.run:
  parse → resolve → interpret
          ↑ 失败则打印错误，不执行
```

---
