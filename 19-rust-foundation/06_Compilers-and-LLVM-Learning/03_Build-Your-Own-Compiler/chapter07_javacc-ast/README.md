# 第 7 章 · JavaCC 的 action 和抽象语法树

> **《自制编译器》** · [03 Build Your Own Compiler](../../README.md) · [本书目录](../../本书目录.md) · 第2部分 · 抽象语法树和中间代码

## 状态

- [x] 已读（笔记整理）

---

## 一句话

**第2部分开篇** — 语法规则只 **检查**；**action `{}`** 在匹配时跑 Java 代码 **建 AST**；**Token** 语义值 · `return` 非终端值；**`ast` 包 Node 层次**（`AST`/`StmtNode`/`ExprNode`…）· `location()`/`dump()`；**手写节点** 弃 JJTree 以保 **静态类型**。

---

## 专项笔记（一小节一文件）

| 小节 | 主题 | 阅读 |
|------|------|------|
| — | 本章定位 | [00-overview.md](./00-overview.md) |
| §1 | JavaCC 的 action | [01-javacc-actions.md](./01-javacc-actions.md) |
| §2 | 抽象语法树和节点 | [02-ast-and-nodes.md](./02-ast-and-nodes.md) |
| — | 速记 · 自测 |

---

## 与仓库其他部分

| 本书 ch7 | 对照 |
|----------|------|
| 第1部分 ch6 | [完整文法](../chapter06_parsing/README.md) — 本章 **填 action** |
| ch8 下一章 | 各类 AST 节点实现细节 |
| ch2 `ast` 包 | [cbc 包结构](../chapter02_cflat-cbc/02-cbc-packages.md) |
| CI / EaC | [CI 表示代码](../../../01_Crafting-Interpreters/part02_jlox/chapter05_representing-code/) · [EaC ch5 IR](../../../02_Compiler-Principles/chapter05_ir/) |

---

## 逻辑脉络

action 机制 → Node 类群与 cbc 设计取舍。

---

## 速记

## 本章速记

```text
§1  action {} · Token(image/位置) · return 节点 · 与 | * + 组合
§2  Node→AST/Stmt/Expr · location/dump · BinaryOpNode · 不用 JJTree
```

---

## 三句背诵

1. **文法匹配检查；action 建 AST。**
2. **终端 = Token；非终端 = return 的 Node。**
3. **cbc 手写节点类，不要 JJTree 的 Node[]。**

---

## 对照表

| 概念 | 一句话 |
|------|--------|
| action | 匹配时执行的 `{ Java }` |
| 语义值 | 符号携带的对象/Token |
| `--dump-ast` | 打印 AST 调试 |
| JJTree | 自动生成但弱类型 |

---

## 自测

- [ ] 如何从 `<IDENTIFIER>` 取名字字符串
- [ ] 重复参数列表 action 为何执行多次
- [ ] StmtNode 与 ExprNode 分工
- [ ] 强类型子字段 vs 子节点数组

---

## 阅读进度

- [x] ch7 JavaCC 的 action 和抽象语法树
- [x] ch8 抽象语法树的生成

