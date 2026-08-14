# 第 7 章 · JavaCC 的 action 和抽象语法树 · §2 抽象语法树和节点

← [本章目录](./README.md) · 上一节：[01-javacc-actions.md](./01-javacc-actions.md) · 下一节：---

## Node 类群与层次

cbc **`ast` 包** — 大量继承 **`Node`** 的类，各表示一种语法单位。

```text
                    Node
                      │
        ┌─────────────┼─────────────┐
        │             │             │
       AST      StmtNode      ExprNode    TypeDefinition …
     (根)         │              │
              IfNode …    BinaryOpNode …
```

| 基类 | 角色 |
|------|------|
| **`AST`** | **整文件** 根 — `compilation_unit` 产物 |
| **`StmtNode`** | 语句 |
| **`ExprNode`** | 表达式 |
| **`TypeDefinition`** | 类型定义相关 |

→ [ch3 AST 概念](../chapter03_parse-overview/01-analysis-phases-and-tokens.md) · ch8 逐类实现。

---

## Node 基类能力

| 方法 | 用途 |
|------|------|
| **`location()`** | 节点对应源码 **位置** — 语义错误 **诊断** |
| **`dump()`** | **文本化** 打印子树结构 |

```bash
cbc --dump-ast hello.cb   # 概念：调试看树形
```

**工程**：前端 Pass 的 **可视化** — 类似 LLVM `view AST` / `rustc -Z unpretty=ast`。

---

## 示例：`BinaryOpNode`

二元运算 **`x + y`**：

| 字段 | 含义 |
|------|------|
| **`left`** | 左 **`ExprNode`** |
| **`right`** | 右 **`ExprNode`** |
| **`operator`** | 运算符（+、-、* …） |

```text
    BinaryOpNode(+)
       /    \
   Var(x)  Var(y)
```

**组合原则**：小节点 **作为字段** 挂到大节点 — 与 ch6 **expr 分层** 一一对应，action 里 `new BinaryOpNode(l, op, r)`。

---

## 为何不用 JJTree

JavaCC 生态 **JJTree** 可 **半自动** 生成 action + 节点。

| JJTree 默认 | cbc 取舍 |
|-------------|----------|
| 子节点统一 **`Node[]`** | **手写专用类** |
| 访问子节点需 **downcast** | **`left`/`right` 强类型字段** |
| 快速原型 | **可读、可维护、类型安全** |

```text
// JJTree 风格（概念）
Node[] children;  ((ExprNode)children[0])  // 繁琐

// cbc 风格
ExprNode left, right;  // 直接访问
```

**结论**：教学 + 工程清晰度 **优先** — 多写类，少魔法。Rust 侧类比：手写 `enum Stmt { … }` vs 泛型 `Vec<Box<dyn Node>>`。

---

## 与后续章节

| 章 | 内容 |
|----|------|
| **ch8** | 表达式/语句/声明 **各类 Node** 在 `.jj` 中如何构造 |
| **ch9～10** | 遍历 AST **语义分析** |
| **ch11** | AST → **IR** |

---

## 自测

- [ ] `AST` vs `ExprNode` 各表示什么
- [ ] `location()` 与 `dump()` 场景
- [ ] 弃 JJTree 的两条理由
- [ ] `BinaryOpNode` 三字段各是什么
