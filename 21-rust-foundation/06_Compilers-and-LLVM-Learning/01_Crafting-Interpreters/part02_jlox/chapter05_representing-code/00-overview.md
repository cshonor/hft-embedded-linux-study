# 第 5 章 · Representing Code（表示代码 / AST） · 本章定位

← [本章目录](./README.md) · 下一节：[01-context-free-grammars.md](./01-context-free-grammars.md)

---

**ch4** 产出线性 **Token 流**；**ch5** 定义如何把 Token 组织成反映**语法嵌套**的树 → **抽象语法树（AST）**。

| 输入（概念上） | 输出（本章） |
|----------------|--------------|
| Token 序列 | **AST 节点类型** + **Visitor** 基础设施 |

**ch6 Parsing** 将把 Token **真正构造**成这棵树。

| 小节 | 主题 |
|------|------|
| **§5.1** | 上下文无关文法（CFG）· BNF · Lox 表达式语法 |
| **§5.2** | Java `Expr` 层次 · `GenerateAst.java` |
| **§5.3** | Visitor · 双重分派 |
| **§5.4** | `AstPrinter` 实战 |

**附录 II**：[`appendix02_generated-ast-classes`](../../backmatter/appendix02_generated-ast-classes/) · 生成代码参考。

---
