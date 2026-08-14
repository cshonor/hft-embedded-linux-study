# 第 11 章 · Resolving and Binding（解析与绑定） · §11.2 语义分析（Semantic Analysis）

← [本章目录](./README.md) · 上一节：[01-static-scope.md](./01-static-scope.md) · 下一节：[03-a-resolver-class.md](./03-a-resolver-class.md)

---

**办法**：在 **AST 构建完成之后、interpret 之前**，增加一趟 **只分析变量绑定** 的遍历。

```text
Source → Scanner → Parser → AST
                              ↓
                         Resolver（语义分析）
                              ↓
                         Interpreter
```

| 特点 | 说明 |
|------|------|
| 不执行用户代码 | 只 walk 树，维护作用域结构 |
| 输出 | 每个 **`Expr.Variable` / `Expr.Assign`** 关联 **栈深度 distance** |
| 对照 ch2 | 编译之山上的 **Analysis** 阶段 |

---
