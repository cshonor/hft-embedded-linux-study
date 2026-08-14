# 第 9 章 · 语义分析（1）引用的消解 · §1 语义分析的概要

← [本章目录](./README.md) · 上一节：[00-overview.md](./00-overview.md) · 下一节：[02-variable-resolution.md](./02-variable-resolution.md)

---

## 语义分析的 5 个阶段（严格顺序）

| 顺序 | 阶段 | 本章 | 章节 |
|:----:|------|:----:|------|
| 1 | **变量引用的消解** | ✓ | **ch9 §2** |
| 2 | **类型名称的消解** | ✓ | **ch9 §3** |
| 3 | **类型定义检查** | | ch10 |
| 4 | **表达式的有效性检查** | | ch10 |
| 5 | **静态类型检查** | | ch10 |

```text
LocalResolver → TypeResolver → … → TypeChecker
     ↑ 必须先知道变量绑哪、类型实体是啥
```

**不能乱序** — 例如未消解变量就不能可靠做类型检查。

→ [ch1 语义分析阶段](../chapter01_start/02-four-compiler-stages.md) · [EaC ch4](../../../02_Compiler-Principles/chapter04_context/README.md)

---

## 为何要遍历 AST

语义 = 对 **每个节点** 施加规则 — 需 **有序访问** 全树。

**反模式**：在每个 `Node` 子类里写 `check()` — 逻辑 **散落**，难维护。

---

## Visitor 模式

| 角色 | cbc |
|------|-----|
| **节点** | `Node` 子类 — `accept(visitor)` |
| **访问者** | 实现 **`ASTVisitor`** — **集中** 操作 |

```text
TypeChecker.visit(BinaryOpNode n) { … }
TypeChecker.visit(IfNode n) { … }
// 一类 Pass 一个 Visitor 实现
```

**收益**：**可读、可维护** — 新增节点类型时改 Visitor 一处（+ 接口方法）。

→ 经典设计模式；LLVM/Clang 用 **`RecursiveASTVisitor`** 同类思路。

---

## cbc 的 Visitor 派生类

| 类 | 职责 |
|----|------|
| **`LocalResolver`** | **变量引用消解** — ch9 §2 |
| **`TypeResolver`** | **类型名称消解** — ch9 §3 |
| **`TypeChecker`** | **静态类型检查** — ch10 |

均实现 **`ASTVisitor`**（或分阶段接口）— **`compile`** 中 **顺序调用**。

```text
AST ast = parse();
new LocalResolver().resolve(ast);
new TypeResolver().resolve(ast);
// … ch10 …
```

---

## 自测

- [ ] 五阶段前两个是什么
- [ ] Visitor 相对「节点内 check」的好处
- [ ] LocalResolver 与 TypeChecker 各在哪一章
