# 第 16 章 · Scanning on Demand（按需扫描） · 本章定位

← [本章目录](./README.md) · 下一节：[01-a-token-at-a-time.md](./01-a-token-at-a-time.md)

---

**clox 编译器前端起步**：重写扫描器。与 jlox **一次性扫描成 Token 列表** 不同，clox **按需返回单个 Token**，省内存、与 **递归下降编译器** 天然契合。

| 对比 | jlox ch4 | clox ch16 |
|------|----------|-----------|
| 输出 | **`List<Token>`** 全文件 | **`scanToken()`** 每次 1 个 |
| 存储 | 堆上 Token 对象列表 | Token **按值** 在 C **栈** 上 |
| 调用者 | Parser 迭代列表 | Compiler **`advance()`** 拉下一个 |
| 关键字 | HashMap / 表查找 | **硬编码 DFA / switch 路径** |

| 小节 | 主题 |
|------|------|
| **§16.1～§16.2** | **A Token at a Time** · 按需 API |
| **§16.3** | **Lexeme** · 空白 · 注释 · 双字符 |
| **§16.4** | 标识符 · **关键字 Trie/DFA** |

---
