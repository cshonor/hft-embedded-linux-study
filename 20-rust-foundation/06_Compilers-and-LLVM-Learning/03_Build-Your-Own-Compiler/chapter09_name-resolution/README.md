# 第 9 章 · 语义分析（1）引用的消解

> **《自制编译器》** · [03 Build Your Own Compiler](../../README.md) · [本书目录](../../本书目录.md) · 第2部分 · 抽象语法树和中间代码

## 状态

- [x] 已读（笔记整理）

---

## 一句话

**语义 Pass 开篇** — AST 建完后 **五阶段有序** 分析；**Visitor** 聚逻辑于 `ASTVisitor` 实现类；本章实现前两步：**`LocalResolver`**（`VariableNode` → 定义 · **Scope 栈/树** 向上查）与 **`TypeResolver`**（**`TypeRef` → `Type`** · **`TypeTable`** · 类型无嵌套作用域）。

---

## 专项笔记（一小节一文件）

| 小节 | 主题 | 阅读 |
|------|------|------|
| — | 本章定位 | [00-overview.md](./00-overview.md) |
| §1 | 语义分析的概要 | [01-semantic-overview-visitor.md](./01-semantic-overview-visitor.md) |
| §2 | 变量引用的消解 | [02-variable-resolution.md](./02-variable-resolution.md) |
| §3 | 类型名称的消解 | [03-type-name-resolution.md](./03-type-name-resolution.md) |
| — | 速记 · 自测 |

---

## 与仓库其他部分

| 本书 ch9 | 对照 |
|----------|------|
| ch8 AST | [chapter08_build-ast](../chapter08_build-ast/README.md) · `VariableNode` · `TypeRef` |
| ch10 下一章 | 类型定义检查 · 表达式有效性 · **静态类型检查** |
| EaC | [ch4 上下文分析](../../../02_Compiler-Principles/chapter04_context/) |
| Rust | `rustc` **name resolution**（Hir）— 同类问题，更强规则 |

---

## 逻辑脉络

五阶段地图 + Visitor → 变量 Scope → 类型 TypeTable。

---

## 速记

## 本章速记

```text
§1  语义五阶段(严序) · Visitor/ASTVisitor · Local/Type/TypeChecker
§2  LocalResolver · Scope栈 · 内→外查 · VariableNode→定义
§3  TypeResolver · TypeTable · TypeRef→Type · 类型无嵌套域
```

---

## 三句背诵

1. **语义分五步；本章做变量名与类型名消解。**
2. **Visitor 把 Pass 逻辑收到 Resolver/Checker 里。**
3. **变量用 Scope 向上查；类型用 TypeTable 查。**

---

## 对照表

| 组件 | 任务 |
|------|------|
| LocalResolver | 变量引用 → 定义 |
| TypeResolver | TypeRef → Type |
| ToplevelScope / LocalScope | 变量作用域 |
| TypeTable | 类型名映射 |

---

## 自测

- [ ] 五阶段顺序及后三步在哪章
- [ ] Visitor 模式解决什么问题
- [ ] 变量查找从内到外过程
- [ ] 类型为何不用 Scope 树

---

## 阅读进度

- [x] ch9 语义分析（1）引用的消解
- [x] ch10 语义分析（2）静态类型检查

