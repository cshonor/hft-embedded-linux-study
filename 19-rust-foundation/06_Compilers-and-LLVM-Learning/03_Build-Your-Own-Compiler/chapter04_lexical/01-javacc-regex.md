# 第 4 章 · 词法分析 · §1 基于 JavaCC 的扫描器描述

← [本章目录](./README.md) · 上一节：[00-overview.md](./00-overview.md) · 下一节：[02-token-unstructured.md](./02-token-unstructured.md)

---

## 扫描器任务

将源码 **切分** 为单词，赋予 **种类 + 语义值** → **token 序列**（[ch3](../chapter03_parse-overview/01-analysis-phases-and-tokens.md)）。

**JavaCC 做法**：在 `.jj` 里用 **正则表达式** 描述各单词的 **词法规则**。

---

## JavaCC 正则支持的模式

| 模式 | 写法示例 | 含义 |
|------|----------|------|
| **固定字符串** | `"int"` | 字面匹配关键字 |
| **字符组** | `["0"-"9"]` | 范围内任一字符 |
| **排除型字符组** | `~[]` | **非** 空集内字符（任意等） |
| **0 或多次** | `*` | Kleene 星 |
| **1 或多次** | `+` | 至少一次 |
| **n～m 次** | `{n,m}` | 有界重复 |
| **可选** | `?` | 0 或 1 次 |
| **多选一** | `\|` | 分支 |

```text
// 概念组合
["0"-"9"]+     →  至少一位数字
"0" ["x""X"] … →  十六进制前缀等（ch2 §2 详述）
```

与 [EaC ch2 正则→NFA/DFA](../../../02_Compiler-Principles/chapter02_scanners/) 同源 — JavaCC 在生成阶段替你 **编译正则** 为扫描代码。

---

## 在 `.jj` 中的位置

```text
options { … }
PARSER_BEGIN … PARSER_END …

TOKEN : { … }    ← 本章主体：词法规则
```

具体 **`TOKEN` / `SKIP` / 状态** 命令见 §2～§4。

---

## 自测

- [ ] 扫描器输入/输出各是什么
- [ ] 举两种 JavaCC 正则构造（字符组 + 重复）
- [ ] 正则与 ch3「token 三要素」如何对应
