# 第 16 章 · Scanning on Demand（按需扫描） · §16.3 处理词素与空白（Lexemes and Whitespace）

← [本章目录](./README.md) · 上一节：[01-a-token-at-a-time.md](./01-a-token-at-a-time.md) · 下一节：[03-identifiers-and-keywords.md](./03-identifiers-and-keywords.md)

---

**扫描循环**（C 手写）：

```text
skipWhitespace():
  空格、制表
  换行 → line++
  // 注释 → 扫到行尾

scanToken():
  skipWhitespace
  start = current
  switch (首字符) { ... }
```

| 类别 | 处理 |
|------|------|
| **单字符** | `( ) { } , . - + ; *` 等 → 直接 Token 类型 |
| **双字符** | **`!` + `=`** → `BANG_EQUAL`；**`=` + `=`** → `EQUAL_EQUAL`；**前瞻 `peek()`** |
| **数字** | 读连续 digit · 可选 `.` 小数部分 → **`NUMBER`** |
| **字符串** | `"..."` 转义处理 → **`STRING`**（完整 lexeme） |

**Token 结构**：

| 字段 | 说明 |
|------|------|
| **`type`** | 枚举 |
| **`start` / `length`** | 指向 **源码缓冲区** 的 lexeme 切片（非拷贝字符串） |
| **`line`** | 行号 |

**Lexeme**：源码中的 **原始字符切片**；关键字与标识符 **词面相同**，靠 §16.4 区分。

---
