# 第 11 章 · Resolving and Binding（解析与绑定） · §11.3 解析器类（A Resolver Class）

← [本章目录](./README.md) · 上一节：[02-semantic-analysis.md](./02-semantic-analysis.md) · 下一节：[04-interpreting-resolved-variables.md](./04-interpreting-resolved-variables.md)

---

**`Resolver`** 实现 **`Expr.Visitor`** + **`Stmt.Visitor`**（与 Interpreter 类似，但**不求值**）。

**核心结构**：**作用域栈**

```text
scopes: Stack<Map<String, VariableState>>
  push  进入 block / function
  pop   离开
  declare(name)   当前层标记「已声明」
  define(name)    当前层标记「已定义」
```

| 变量状态 | 含义 |
|----------|------|
| **Declared** | `var x` 已解析，尚未执行到 initializer |
| **Defined** | 已可读写 |

**遍历要点**：

- **`Stmt.Var`**：`declare` → 递归 initializer → `define`。
- **`Stmt.Function`**：新 scope → declare 函数名 → 解析参数与 body → `define` 函数名。
- **`Expr.Variable` / `Assign`**：在当前栈中 **resolve**，计算 **distance**（向外几层 `enclosing`）。

**存储 binding**：Interpreter 侧用 **`Map<Expr, Integer> locals`**（或类似）记录每个变量表达式节点的 **distance**。

---
