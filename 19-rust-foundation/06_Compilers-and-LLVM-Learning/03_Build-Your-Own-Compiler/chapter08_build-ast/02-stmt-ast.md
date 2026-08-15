# 第 8 章 · 抽象语法树的生成 · §2 语句的抽象语法树

← [本章目录](./README.md) · 上一节：[01-expr-ast.md](./01-expr-ast.md) · 下一节：[03-decl-ast.md](./03-decl-ast.md)

---

## if 语句 · `IfNode`

| 字段（概念） | 内容 |
|--------------|------|
| 条件 | **`ExprNode`** |
| then | **`StmtNode`** |
| else | 可选 **`StmtNode`** |

```text
if (e) s1; else s2;  →  IfNode(e, s1, s2)
```

与 [ch6 LOOKAHEAD(1) else](../chapter06_parsing/02-statements.md) 配对 — 树结构绑定 **最内层 if**。

---

## while 语句 · `WhileNode`

| 字段 | 内容 |
|------|------|
| 条件 | **`ExprNode`** |
| 体 | **`StmtNode`**（常 `BlockNode`） |

```text
while (e) { … }  →  WhileNode(e, body)
```

---

## 程序块 · `BlockNode`

**`{ … }`** — C♭ 块内两类子内容：

| 组成 | 说明 |
|------|------|
| **局部变量声明列表** | 块内 `defvars` |
| **语句列表** | `stmts` |

```text
BlockNode
  ├── locals: List<DefinedVariable>
  └── stmts:  List<StmtNode>
```

**作用域**：ch9 语义分析在 **Block 边界** 压栈/弹栈符号表 — 块节点携带声明便于 **一轮遍历** 处理。

---

## 其他语句（概念）

ch6 **13 种 `stmt`** — 各自 `StmtNode` 子类：

| 示例 | 节点 |
|------|------|
| `return expr;` | `ReturnNode` |
| `break;` / `continue;` | `BreakNode` / `ContinueNode` |
| 表达式语句 | `ExprStmtNode` |
| 空语句 | `EmptyNode` |

→ 模式统一：**产生式末尾 `{ return new …Node(…); }`**。

---

## 自测

- [ ] IfNode 三个逻辑部分
- [ ] BlockNode 在 C♭ 比「纯语句列表」多什么
- [ ] while 体为何常为 BlockNode
