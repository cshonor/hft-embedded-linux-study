# 第 4 章 · Scanning（扫描 / 词法分析） · §4.6 更长的词素（Longer Lexemes）

← [本章目录](./README.md) · 上一节：[05-recognizing-lexemes.md](./05-recognizing-lexemes.md) · 下一节：[07-reserved-words-and-identifiers.md](./07-reserved-words-and-identifiers.md)

---

### 注释与空白

| 情况 | 处理 |
|------|------|
| `//` | 行注释 → 消耗到**行尾** |
| 空格 / `\r` / `\t` | **跳过** |
| `\n` | 跳过并 **`line++`** |

### 字符串字面量

- 从 `"` 开始，读到**闭合 `"`**（Lox 支持**多行**字符串）。
- Token 的 literal：用 **`substring`** 去掉外围引号，得到真实字符串。

### 数字字面量

- Lox 只有 **双精度浮点**（`double` / Java `Double`）。
- 小数点：用 **`peekNext()`** 两步前瞻，确保 `.` 后面还有数字（区分 `123.` 与 `123.45`）。
- 解析：Java 内置字符串 → `Double` 转换。

---
