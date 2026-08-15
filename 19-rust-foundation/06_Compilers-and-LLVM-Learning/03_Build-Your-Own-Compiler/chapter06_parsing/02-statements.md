# 第 6 章 · 语法分析 · §2 语句的分析

← [本章目录](./README.md) · 上一节：[01-definitions.md](./01-definitions.md) · 下一节：[03-expressions.md](./03-expressions.md)

---

## 语句列表与单条语句

| 非终端 | 含义 |
|--------|------|
| **`stmts`** | **语句列表**（块内或多条连续语句） |
| **`stmt`** | **单条语句** — **13 种**选项之一 |

```text
stmts : ( stmt )*
stmt  : 以下之一 …
```

---

## `stmt` 的 13 种（概念分类）

| 类别 | 示例 |
|------|------|
| **空语句** | `;` |
| **块** | `block` `{ … }` |
| **条件** | `if` |
| **循环** | `while` · `for` |
| **跳转** | `return` · `break` · `continue` · `goto` … |
| **表达式语句** | `expr;` |
| **其他** | 书中列齐 13 种 — 覆盖 C♭ 控制流子集 |

函数体、if/while 内部均递归调用 **`stmts`** / **`block`**。

---

## `if` 与 dangling else

**问题**：[ch5](../chapter05_javacc-parser/02-ambiguity-and-lookahead.md) — 嵌套 `if` 时 `else` 归属。

**C♭ 文法解法**：在内层 `if` 的可选 `else` 分支前加 **`LOOKAHEAD(1)`**：

```text
// 概念
stmt :
  <IF> "(" expr() ")" stmt()
  [ LOOKAHEAD(1) <ELSE> stmt() ]   // else 强制匹配最近未闭合 if
| …
```

| 无 LOOKAHEAD | 有 LOOKAHEAD(1) |
|--------------|-----------------|
| 可能选错分支 / 冲突 | `else` **绑最内侧 if** — 与 C 语义一致 |

**实践要点**：ch5 理论 → ch6 **第一条硬规则** — 读 cbc `.jj` 时搜索 `LOOKAHEAD` 与 `<ELSE>`。

---

## 与 ch7～8

语句节点（`IfNode`、`WhileNode`…）在 **action** 里构造 — ch8 **语句 AST**。

---

## 自测

- [ ] `stmts` vs `stmt`
- [ ] 为何 if 规则必须处理 else
- [ ] LOOKAHEAD(1) 在 if 产生式中的位置（概念）
