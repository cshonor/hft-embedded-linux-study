# 第 11 章 · Resolving and Binding（解析与绑定） · Resolver 与 Interpreter 分工

← [本章目录](./README.md) · 上一节：[05-resolution-errors.md](./05-resolution-errors.md) · 下一节：---

```text
        Resolver                    Interpreter
        ────────                    ───────────
输入    AST                         AST + locals map
关心    作用域嵌套、声明顺序         值、控制流、调用
输出    locals[expr]=distance       副作用 + 返回值
```

---
