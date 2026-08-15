# 第 4 章 · Scanning（扫描 / 词法分析） · 本章定位

← [本章目录](./README.md) · 下一节：[01-the-interpreter-framework.md](./01-the-interpreter-framework.md)

---

**Part II 第一段代码**：实现 **jlox 扫描器（Scanner）**——编译/解释流水线的**第一站**（[ch02 §2.1.1](../../part01_welcome/chapter02_map-of-the-territory/README.md)）。

| 输入 | 输出 |
|------|------|
| 源代码**字符流** | **Token** 序列（线性） |

下一章 **ch5 Representing Code** 将把 Token 流解析成 **AST**（树状）。

| 小节 | 主题 |
|------|------|
| **§4.1** | jlox 脚手架：`runFile` / REPL / `error()` |
| **§4.2** | Lexeme vs Token |
| **§4.3** | 正则语言、手写扫描循环 |
| **§4.4～4.7** | `Scanner` 类：指针、单/双字符、注释/字符串/数字、保留字 |

---
