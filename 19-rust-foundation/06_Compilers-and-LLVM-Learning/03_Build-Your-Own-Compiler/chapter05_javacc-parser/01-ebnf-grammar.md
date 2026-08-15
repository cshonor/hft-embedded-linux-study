# 第 5 章 · 基于 JavaCC 的解析器的描述 · §1 基于 EBNF 语法的描述

← [本章目录](./README.md) · 上一节：[00-overview.md](./00-overview.md) · 下一节：[02-ambiguity-and-lookahead.md](./02-ambiguity-and-lookahead.md)

---

## 解析器的核心作用

| 组件 | 输入 → 输出 |
|------|-------------|
| **Scanner**（ch4） | 字符 → **token** |
| **Parser**（本章） | **token 序列** → 高级单位 + **语法树** |

高级单位示例：**语句**、**表达式**、**函数调用** — 各由 **多个 token** 按文法组合而成。

```text
token:  if ( x ) { y ; }
         ↓ Parser
tree:   IfStmt( cond=x, body=Block(y) )
```

→ [ch3 AST](../chapter03_parse-overview/01-analysis-phases-and-tokens.md) · ch7 起挂 **语义动作** 建 cbc AST。

---

## 终端符与非终端符

| 种类 | JavaCC 中 | 含义 |
|------|-----------|------|
| **终端符** | `<IDENTIFIER>`、`"="`、`";"` … | **树的叶子** — 来自词法 |
| **非终端符** | `stmt()`、`expr()`、`primary()` … | **语法规则** — 由子符号组成 |

```text
// 概念 EBNF（JavaCC 风格）
void stmt() :
{
  <IF> expr() <THEN> stmt() [ <ELSE> stmt() ]
| …
}
```

**非终端** = 产生式左侧/规则名；**终端** = TOKEN 或字面串。

---

## EBNF 表示法（JavaCC）

| 构造 | 写法 | 含义 |
|------|------|------|
| **连接** | `A B C` | **顺序**出现 |
| **重复 0+** | `X*` | 零次或多次 |
| **重复 1+** | `X+` | 一次或多次 |
| **选择** | `A \| B \| C` | 多选一 |
| **可省略 0/1** | `[ X ]` | 可选 |

```text
// 表达式列表（概念）
argList(): { expr() ( "," expr() )* }

// 等价：连接 + * 重复
```

与 [CI ch5 CFG/EBNF](../../../01_Crafting-Interpreters/part02_jlox/chapter05_representing-code/01-context-free-grammars.md) 同一套符号；JavaCC 生成 **LL 递归下降** 代码。

---

## 与 ch6 分工

| 章 | 内容 |
|----|------|
| **ch5** | EBNF **写法** + **冲突处理** |
| **ch6** | C♭ **定义 / 语句 / 表达式 / 项** 完整产生式 |

---

## 自测

- [ ] Scanner 与 Parser 各识别什么层次
- [ ] 终端符 vs 非终端符各举两例
- [ ] `*`、`+`、`|`、`[]` 各表示什么
