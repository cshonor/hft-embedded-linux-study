# 第 5 章 · Representing Code（表示代码 / AST） · §5.2 实现语法树（Implementing Syntax Trees）

← [本章目录](./README.md) · 上一节：[01-context-free-grammars.md](./01-context-free-grammars.md) · 下一节：[03-working-with-trees.md](./03-working-with-trees.md)

---

### Expr 类层次

- 基类抽象 **`Expr`**。
- 每种语法形式一个**子类**，例如：

| 节点类 | 对应语法 |
|--------|----------|
| `Binary` | 二元运算 `left op right` |
| `Unary` | 一元运算 `op right` |
| `Literal` | 字面量 |
| … | ch6 后还会扩展（分组、变量等） |

### 元编程：`GenerateAst.java`

- 手写大量字段 + 构造函数的 AST 类**繁琐且易错**。
- 作者用 **几十行 Java 脚本** `GenerateAst.java` **生成**全部 AST 源码 → 「让机器写无聊代码」。
- 运行脚本 → 输出 `Expr.java` 等文件（附录 II 含生成结果）。

```text
BNF 规则（人读）  →  GenerateAst（机器写）  →  Java AST 类（编译用）
```

**Rust 对照**：`syn` / 手写 enum · **proc-macro** 生成 AST —— 同一「元编程减样板」思路（RFR ch7）。

---
