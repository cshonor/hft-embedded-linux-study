# 第 16 章 · Scanning on Demand（按需扫描） · §16.1～§16.2 按需生成词法单元（A Token at a Time）

← [本章目录](./README.md) · 上一节：[00-overview.md](./00-overview.md) · 下一节：[02-lexemes-and-whitespace.md](./02-lexemes-and-whitespace.md)

---

**全局扫描状态**（典型）：

| 变量 | 作用 |
|------|------|
| **`scanner.start`** | 当前 lexeme 起始 |
| **`scanner.current`** | 扫描头 |
| **`scanner.line`** | 当前行号（供 ch14 **`lines[]`**） |

**API**：

```c
Token scanToken();   // 跳过空白/注释后，识别下一个 Token
bool  isAtEnd();     // 是否 EOF
```

**编译器侧**：

```c
Token current = advance();   // scanToken + 缓存
Token peek();                // lookahead 不消费
```

| 优点 | 说明 |
|------|------|
| **零 Token 列表** | 大文件也不预分配 |
| **流水线** | Scan → Parse/Compile 交织（Parser 要 Token 才扫） |
| **缓存友好** | 顺序读源码，局部性更好 |

**对照 jlox**：[ch04 Scanning](../../part02_jlox/chapter04_scanning/README.md) 的 **`Scanner.scanTokens()`** 一次扫完。

---
