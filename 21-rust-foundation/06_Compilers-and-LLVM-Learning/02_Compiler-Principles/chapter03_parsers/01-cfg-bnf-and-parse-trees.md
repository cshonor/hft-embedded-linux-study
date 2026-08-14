# 第 3 章 · 语法分析 · §1 CFG · BNF · 分析树 · 歧义

← [本章目录](./README.md) · 上一节：[00-overview.md](./00-overview.md) · 下一节：[02-top-down-parsing.md](./02-top-down-parsing.md)

---

## 上下文无关文法（CFG）

精确、计算机可读地描述编程语言**语法**的标准工具。

### BNF（Backus-Naur Form）组成

| 成分 | 含义 |
|------|------|
| **终结符** | 单词 — 对应 Scanner 输出的 **Token** |
| **非终结符** | 语法变量 — 如 `expr`、`stmt`、`assignment` |
| **产生式** | 重写规则 — `A → α β γ` |
| **开始符号** | 分析树根 — 通常 `program` 或 `translation_unit` |

```bnf
assignment → identifier "=" expression
expression → expression "+" term | term
```

---

## 派生与分析树（Parse Tree）

| 概念 | 说明 |
|------|------|
| **派生（Derivation）** | 反复应用产生式，从开始符号推出输入串 |
| **分析树** | 派生的树形记录 — **叶子** = 终结符（Token），**内部节点** = 非终结符 |

```text
        assignment
       /    |     \
   ident   "="   expression
                  /    |    \
              expr  "+"  term
```

实际编译器常把分析树**简化为 AST**（去掉纯语法糖节点）。

→ [CI jlox ch5 AST](../../../01_Crafting-Interpreters/part02_jlox/chapter05_representing-code/README.md)

---

## 歧义性（Ambiguity）

**同一输入**可对应**多棵不同分析树** → 文法**有歧义**。

| 经典例 | 问题 |
|--------|------|
| **`if-then-else` 悬挂** | `else` 匹配哪个 `if`？ |
| **表达式优先级** | `a + b * c` 先加还是先乘？ |

### 消除手段

- **重写文法** — 引入层级非终结符（`expr` / `term` / `factor`）固定**优先级与结合性**。
- **优先级声明** — YACC/Bison 的 `%left` / `%right`（工程 shortcut）。

> 扫描器（RE）**不能**表达嵌套括号计数；CFG **可以** — 这是 ch2 vs ch3 的分界。

---

## 与 ch2 衔接

| ch2 | ch3 |
|-----|-----|
| Token 流 | CFG 的终结符输入 |
| 正则语言 | 上下文无关语言（更 expressive） |
