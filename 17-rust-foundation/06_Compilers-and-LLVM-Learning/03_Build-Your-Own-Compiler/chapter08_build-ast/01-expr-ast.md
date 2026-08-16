# 第 8 章 · 抽象语法树的生成 · §1 表达式的抽象语法树

← [本章目录](./README.md) · 上一节：[00-overview.md](./00-overview.md) · 下一节：[02-stmt-ast.md](./02-stmt-ast.md)

---

## 字面量与基本变量（primary）

| 源码 | 节点（概念） |
|------|--------------|
| 整数字面量 | **`IntegerLiteralNode`** |
| 字符/字符串 | **`StringLiteralNode`** 等 |
| 标识符 | **`VariableNode`** |

action 在 [ch6 `primary`](../chapter06_parsing/04-terms.md) 匹配处 **`return new …`**。

```text
42     → IntegerLiteralNode(42)
"x"    → StringLiteralNode(…)
foo    → VariableNode("foo")
```

---

## TypeRef 与 Type

cbc **刻意拆分**：

| 类 | 含义 |
|----|------|
| **`TypeRef`** | 语法上的 **类型名称引用** — 尚未绑定实体 |
| **`Type`** | **类型实体** — 定义完成后 |

**动机**：类型 **先被引用、后定义**（相互依赖 struct 等）— ch9 引用消解时再 **Ref → Type**。

→ 类似 C 编译器「不完整类型」阶段；Rust 由 **name resolution** 一次性处理。

---

## 一元运算

**前置/后置** `++`、`--`、`*`、`&` 等 → **`UnaryOpNode`**（及变体）。

```text
* p   → UnaryOpNode(DEREF, p)
i++   → UnaryOpNode(POST_INC, i)   // postfix 在 postfix 规则 action
```

与 ch6 **`unary` / `postfix`** 产生式一一挂 action。

---

## 二元运算 · 左结合

`+` `-` `*` 等 **左结合** — `a - b - c` = `(a - b) - c`。

JavaCC：**重复模式 `*`** 生成 **左深** 嵌套 `BinaryOpNode`：

```text
// 概念
expr = term ( op term )*
// 每次 { lhs = new BinaryOpNode(lhs, op, rhs); }
```

```text
a - b - c

    -
   / \
  -   c
 / \
a   b
```

---

## 赋值 · 右结合

**`=` 右结合** — `i = j = 1` ≡ `i = (j = 1)`。

| 左结合 | 右结合 |
|--------|--------|
| `*` 循环叠左边 | **`=` 用规则递归** |

```text
// 概念：expr 层
lhs = expr10()
[ "=" rhs = expr() { return new AssignNode(lhs, rhs); } ]
// 右侧再次调用 expr() → 右结合树
```

```text
i = j = 1

  =
 / \
i   =
   / \
  j   1
```

→ [ch6 最外层 `expr`](../chapter06_parsing/03-expressions.md) 赋值在最顶层的体现。

---

## 自测

- [ ] primary 三类节点
- [ ] TypeRef vs Type 为何分开
- [ ] 左结合用 `*`，右结合用递归 — 各举一例
