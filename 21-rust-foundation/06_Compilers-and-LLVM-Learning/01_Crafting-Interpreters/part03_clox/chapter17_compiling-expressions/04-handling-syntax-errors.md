# 第 17 章 · Compiling Expressions（编译表达式） · 语法错误处理（Handling Syntax Errors）

← [本章目录](./README.md) · 上一节：[03-emitting-bytecode.md](./03-emitting-bytecode.md) · 下一节：[05-ast.md](./05-ast.md)

---

C **无异常** → 编译错误用 **标志 + 同步** 恢复。

| 机制 | 说明 |
|------|------|
| **`hadError`** | 本次 compile 是否失败 |
| **`panicMode`** | 已报错 → **抑制级联错误** |
| **`error(message)`** | 打印行号 + 消息；置位上述标志 |
| **`synchronize()`** | 恐慌模式下 **跳到语句边界**（`;`、`}`、class/fun/for/if/var/while 等） |

```text
语法错误 → panicMode = true
  → 后续 error 被忽略
  → synchronize() 找到安全同步点
  → panicMode = false，继续解析（尽量多报独立错误）
```

**对照 jlox**：Java **`ParseError`** + throw；clox **不 longjmp**，靠状态机恢复。

---
