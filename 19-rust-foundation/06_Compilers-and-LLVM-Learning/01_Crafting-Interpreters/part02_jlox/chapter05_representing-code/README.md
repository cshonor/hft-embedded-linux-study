# 第 5 章 · Representing Code（表示代码 / AST）

> **Crafting Interpreters** · [Part II · jlox](../README.md) · [本书目录](../../本书目录.md)  
> 在线：[craftinginterpreters.com](https://craftinginterpreters.com/representing-code.html) · [中文在线](https://craftinginterpreters.com/representing-code.html)

## 状态

- [x] 已读（笔记整理）

---

## 一句话

ch4 产出线性 Token 流；ch5 定义如何把 Token 组织成反映语法嵌套的树 → 抽象语法树（AST）。

---

## 专项笔记（一小节一文件）

| 小节 | 主题 | 阅读 |
|------|------|------|
| — | 本章定位 | [00-overview.md](./00-overview.md) |
| §5.1 | 上下文无关文法（Context-Free Grammars） | [01-context-free-grammars.md](./01-context-free-grammars.md) |
| §5.2 | 实现语法树（Implementing Syntax Trees） | [02-implementing-syntax-trees.md](./02-implementing-syntax-trees.md) |
| §5.3 | 操作树（Working with Trees） | [03-working-with-trees.md](./03-working-with-trees.md) |
| §5.4 | 一个（不太）漂亮的打印机（AstPrinter） | [04-ast.md](./04-ast.md) |
| ·6 | 流水线位置 | [05-ast.md](./05-ast.md) |
| — | 速记 · 自测 · 进度 |

---

## 逻辑脉络


---

## 速记

## 本章速记

```text
§5.1  正则不够 → CFG/BNF · | * + ?
§5.2  Expr 子类 · GenerateAst 生成代码
§5.3  Visitor + 双重分派 · 操作与节点分离
§5.4  AstPrinter → (+ 1 (* 2 3)) 验 parser
```

---

---

## 读后下一步

| 章 | 目录 | 内容 |
|:--:|------|------|
| **6** | [chapter06 · Parsing Expressions](../chapter06_parsing-expressions/) | **递归下降** · Token → AST |
| **7** | Evaluating | Visitor **解释执行** |

动手 ch6 前：运行 **`GenerateAst`** 生成 AST 类 · 扫一眼 **附录 II**。

---

---

## 自测 / 对照（可选）

- [ ] 用 BNF 写一条 Lox **二元表达式**规则（含 `*` 重复）。
- [ ] 解释：为何 `1 + (2 * 3)` 不能只用正则扫描器完成，还需要 parser？
- [ ] 画双重分派：`Binary.accept(interpreter)` 如何调到 `Interpreter.visitBinary`？
- [ ] 对 `print 1 + 2;` 的预期 AstPrinter 输出是什么？（需 ch6 后才有完整语句树）

---

---

## 阅读进度

- [x] §5.1～§5.4 结构梳理（本章笔记）
- [ ] 运行 `GenerateAst.java` / 对照附录 II
- [ ] 实现或阅读 `AstPrinter`
- [ ] 本章 Challenges

