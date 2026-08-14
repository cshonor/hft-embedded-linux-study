# 第 5 章 · Representing Code（表示代码 / AST） · §5.3 操作树（Working with Trees）

← [本章目录](./README.md) · 上一节：[02-implementing-syntax-trees.md](./02-implementing-syntax-trees.md) · 下一节：[04-ast.md](./04-ast.md)

---

解释器生命周期里要对 AST 做多种**操作**：

| 操作示例 | 章节 |
|----------|------|
| **解释执行** | ch7 Evaluating |
| 类型检查 | （Lox 动态类型，运行时做） |
| 打印 / 调试 | §5.4 AstPrinter |

若把每种逻辑都写进节点类 → **臃肿、难扩展**。

### 访问者模式（Visitor Pattern）

- 把「对每种节点的操作」抽到**独立的 Visitor 类**。
- AST 节点只负责 **`accept(visitor)`** → 回调到 `visitor.visitXxx(this)`。

### 双重分派（Double Dispatch）

1. 第一次分派：`expr.accept(visitor)` → 根据**节点类型**选方法。
2. 第二次分派：`visitor.visitBinary(node)` → 根据 **Visitor 实现**选行为。

```text
新增一种操作  →  新建 Visitor 子类  →  不改 AST 节点类
新增一种节点  →  改 Expr + GenerateAst + 所有 Visitor  （较少发生）
```

**本仓库**：编译器里常见 **IR Visitor** · LLVM Pass 遍历 SSA —— 同族「树/图 + 访问」思想。

---
