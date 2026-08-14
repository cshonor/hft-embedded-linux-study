# 第 4 章 · Scanning（扫描 / 词法分析） · §4.5 识别词素（Recognizing Lexemes）

← [本章目录](./README.md) · 上一节：[04-the-scanner-class.md](./04-the-scanner-class.md) · 下一节：[06-longer-lexemes.md](./06-longer-lexemes.md)

---

### 单字符词素

- 用 **`switch`** 识别 `( ) { } , . - + ; *` 等 → 直接 `addToken(类型)`。

### 词法错误（Lexical errors）

- 遇到 Lox **不允许**的字符（如 `@`）→ **报错**。
- 关键：**继续扫描**剩余源码 → 一次展示**多个**错误，避免「打地鼠式」修一个报一个。

### 双字符运算符与前瞻（Lookahead）

- `!=`、`==`、`>=` 等需看**下一个**字符。
- **`match(expected)`**：若 `current` 处是 `expected` 则消耗并返回 true。
- 例：见到 `!` → `match('=')` 成功则 `BANG_EQUAL`，否则 `BANG`。

```text
!     → peek ? =  →  !=  vs  !
```

**clox 预告**：Part III **ch16** 同样手写 C 版 Scanner，思路一致。

---
