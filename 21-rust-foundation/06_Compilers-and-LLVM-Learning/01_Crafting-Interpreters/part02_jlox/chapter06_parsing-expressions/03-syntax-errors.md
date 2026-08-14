# 第 6 章 · Parsing Expressions（解析表达式） · §6.3 语法错误（Syntax Errors）

← [本章目录](./README.md) · 上一节：[02-recursive-descent-parsing.md](./02-recursive-descent-parsing.md) · 下一节：[04-wiring-up-the-parser.md](./04-wiring-up-the-parser.md)

---

解析器遇到非法源码时的**两个目标**（略矛盾，需平衡）：

| 目标 | 含义 |
|------|------|
| **多报独立错误** | 一次编译尽量列出多处真实错误 |
| **少级联错误** | 不因前一个错误状态错乱而刷一堆「幽灵错误」 |

### 恐慌模式（Panic mode）

- 遇到**意料之外 Token** → 抛 **`ParseError`**。
- **停止**当前语法树分支的构建，进入恐慌。

### 同步（Synchronization）

- **捕获** `ParseError` → **丢弃 Token** 直到**同步点**：
  - 分号 **`;`**
  - 或语句起始关键字：`class` `fun` `var` `for` `if` `while` `print` …
- 状态重置 → **继续**解析下一条语句，寻找后续真实错误。

```text
错误 → panic → synchronize() → 丢弃直到边界 → 继续 parse
```

**本仓库**：`rustc` 错误恢复更复杂；Lox 教学版 panic mode 是经典入门模板。

---
