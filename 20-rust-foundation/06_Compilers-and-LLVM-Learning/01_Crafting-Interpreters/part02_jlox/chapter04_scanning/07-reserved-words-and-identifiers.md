# 第 4 章 · Scanning（扫描 / 词法分析） · §4.7 保留字与标识符（Reserved Words and Identifiers）

← [本章目录](./README.md) · 上一节：[06-longer-lexemes.md](./06-longer-lexemes.md) · 下一节：[08-scanning.md](./08-scanning.md)

---

### 最大吞噬（Maximal munch）

- 多条规则都能匹配时 → 匹配**字符最多**的那条。
- 例：`orchid` 是**标识符**，不会因前缀 `or` 被当成 **`or` 关键字**。

### 实现要点

1. 以**字母或 `_`** 开头 → 进入标识符/关键字路径。
2. 持续消耗后续**字母与数字**。
3. 得到完整字符串 → 查 **保留字 HashMap**（`and`、`class`、`fun`…）。
4. 命中 → **关键字 Token**；未命中 → **IDENTIFIER**。

| 类别 | 示例 Token 类型 |
|------|-----------------|
| 关键字 | `VAR`、`FUN`、`CLASS`、`IF`… |
| 标识符 | `IDENTIFIER` + lexeme 文本 |

---
