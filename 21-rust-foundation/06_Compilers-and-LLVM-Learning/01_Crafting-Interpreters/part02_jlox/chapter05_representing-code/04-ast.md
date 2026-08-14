# 第 5 章 · Representing Code（表示代码 / AST） · §5.4 一个（不太）漂亮的打印机（AstPrinter）

← [本章目录](./README.md) · 上一节：[03-working-with-trees.md](./03-working-with-trees.md) · 下一节：[05-ast.md](./05-ast.md)

---

- 实现 **Visitor 接口** 的 **`AstPrinter`**。
- 遍历 AST，输出 **Lisp 风格**带括号字符串：

```text
1 + 2 * 3   →   (+ 1 (* 2 3))
```

| 用途 | 说明 |
|------|------|
| 解释执行 | **非必需** |
| **调试 parser** | **极有用** —— 验证 ch6 是否建对了树 |

**建议**：写 ch6 时随时 `new AstPrinter().print(expr)` 对照预期。

---
