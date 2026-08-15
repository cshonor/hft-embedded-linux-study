# 第 6 章 · 语法分析

> **《自制编译器》** · [03 Build Your Own Compiler](../../README.md) · [本书目录](../../本书目录.md) · 第1部分 · 代码分析

## 状态

- [x] 已读（笔记整理）

---

## 一句话

**C♭ 文法落地** — 自上而下四类单位：**定义**（`compilation_unit` · import · defun/defvars/struct/typedef · **后置类型修饰**）→ **语句**（`stmt` 13 种 · **LOOKAHEAD(1) else**）→ **表达式**（`expr`…`expr1` **分层优先级**）→ **项**（`unary`/`postfix`/`primary`）。

---

## 专项笔记（一小节一文件）

| 小节 | 主题 | 阅读 |
|------|------|------|
| — | 本章定位 | [00-overview.md](./00-overview.md) |
| §1 | 定义的分析 | [01-definitions.md](./01-definitions.md) |
| §2 | 语句的分析 | [02-statements.md](./02-statements.md) |
| §3 | 表达式的分析 | [03-expressions.md](./03-expressions.md) |
| §4 | 项的分析 | [04-terms.md](./04-terms.md) |
| — | 速记 · 自测 |

---

## 与仓库其他部分

| 本书 ch6 | 对照 |
|----------|------|
| ch4～5 | [词法](../chapter04_lexical/) · [EBNF/LOOKAHEAD](../chapter05_javacc-parser/) |
| ch7 下一部分 | JavaCC **action** 建 AST |
| CI | [ch6～7 解析/表达式](../../../01_Crafting-Interpreters/part02_jlox/) |
| EaC | [ch3 语法分析](../../../02_Compiler-Principles/chapter03_parsers/) |

---

## 逻辑脉络

文件/定义 → 语句 → 表达式优先级塔 → 一元/后缀/primary。

---

## 速记

## 本章速记

```text
§1  compilation_unit = import* + top_defs · 后置 * [] · defun/defvars/struct/typedef
§2  stmts/stmt(13) · if + LOOKAHEAD(1) else
§3  expr→expr10→…→expr1 优先级塔
§4  unary → postfix* → primary
```

---

## 三句背诵

1. **文件：import + 顶层定义；C♭ 类型修饰符后置。**
2. **语句多分支；if 的 else 用 LOOKAHEAD(1)。**
3. **表达式分层；项分 unary/postfix/primary。**

---

## 非终端地图

| 层级 | 代表规则 |
|------|----------|
| 文件 | `compilation_unit` |
| 语句 | `stmts` · `stmt` |
| 表达式 | `expr` … `expr1` |
| 项 | `unary` · `postfix` · `primary` |

---

## 自测

- [ ] 四种 top_defs
- [ ] dangling else 在 ch6 哪条规则解决
- [ ] 优先级为何用多层 expr 而非单规则
- [ ] postfix 链如何解析 `f()[0]`

---

## 阅读进度

- [x] ch6 语法分析 — **第1部分 完成**
- [x] ch7 JavaCC 的 action 和抽象语法树

