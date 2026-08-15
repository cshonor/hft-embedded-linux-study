# 第 5 章 · 基于 JavaCC 的解析器的描述 · §2 语法的二义性与 token 超前扫描

← [本章目录](./README.md) · 上一节：[01-ebnf-grammar.md](./01-ebnf-grammar.md) · 下一节：---

## 语法的二义性

同一 token 序列可能对应 **多种 parse 树** — 文法 **二义**。

**经典例：dangling else（空悬 else）**

```c
if (a) if (b) x; else y;
// else 归属内层 if 还是外层 if？
```

C 规定 **else 与最近未配对 if** — 文法需 **编码这一偏好**（见下 LOOKAHEAD）。

→ [EaC ch3 二义性](../../../02_Compiler-Principles/chapter03_parsers/) · CI [ch6 解析策略](../../../01_Crafting-Interpreters/part02_jlox/chapter06_parsing-strategies/)

---

## 选择冲突（Choice Conflict）

JavaCC 处理 `|` 分支时，默认 **LOOKAHEAD(1)** — 只看 **1 个 token** 选支。

| 情况 | 结果 |
|------|------|
| 各分支 **首 token 不同** | 可区分 ✓ |
| **多分支同开头** | **选择冲突** ✗ — 生成警告/错误 |

```text
A:  "if" …
|   "if" …     ← 首 token 同为 if → 冲突
```

---

## 解决冲突的方法

### 1. 提取左侧共通部分

把 **共有前缀** 提出，分支只留 **差异后缀** — **左因子分解**。

```text
// 概念：先读共有 if ( expr ) stmt
// 再可选 [ else stmt ]  — 减少同首分支
```

与编译教科书 **消除左公因子** 同思想。

---

### 2. LOOKAHEAD(n)

在分支前加 **`LOOKAHEAD(n)`** — 决策前多读 **n 个 token**，用 **后续非共通** 符号区分。

```text
// dangling else（概念）
if ( … ) stmt() [ LOOKAHEAD(1) <ELSE> stmt() ]
```

**`LOOKAHEAD(1)` 在 `[]` 可选前**：见到 `else` 时 **优先闭合最内层 if** — 实现 C 的 else 绑定规则，避免错误归约到外层。

---

### 3. 省略与重复的冲突

| 场景 | LOOKAHEAD 用途 |
|------|----------------|
| **可选 `[ else stmt ]`** | 见上 — 控制 else 归属 |
| **`*` / `+` 重复** | 多读 token 判断 **继续循环** 还是 **退出**（与下一产生式首集区分） |

LL 解析 **无法自动** 解决所有冲突 — 作者需在 `.jj` **显式** 标注。

---

### 4. 更灵活的 LOOKAHEAD（语法谓词）

`LOOKAHEAD` 括号内 **不限于整数 n** — 可写 **一段完整语法规则**：

```text
LOOKAHEAD( expr() "<" )
```

含义：若 **向前扫描的 token 能匹配** 括号内规则 → 本分支可选。

| 数字 n | 固定多看 n 个 token |
| 规则 | token 数量 **不固定**（含非终端）时仍可做 **结构化预判** |

**代价**：规则复杂时难维护 — 优先 **提公因子**，必要时再上语法 LOOKAHEAD。

---

## 工程 checklist

```text
1. javacc 报 Choice conflict → 看冲突分支首 token
2. 能否左因子分解？
3. 否 → LOOKAHEAD(n) 或 LOOKAHEAD( 产生式 )
4. dangling else → 内层 if 的 [ else ] 前 LOOKAHEAD(1)
```

---

## 自测

- [ ] dangling else 为何二义
- [ ] 选择冲突的默认 lookahead 是几
- [ ] 提公因子 vs LOOKAHEAD 各何时用
- [ ] LOOKAHEAD 内写规则解决什么问题
