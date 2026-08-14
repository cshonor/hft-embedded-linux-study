# 第 4 章 · Scanning（扫描 / 词法分析）

> **Crafting Interpreters** · [Part II · jlox](../README.md) · [本书目录](../../本书目录.md)  
> 在线：[craftinginterpreters.com](https://craftinginterpreters.com/scanning.html) · [中文在线](https://craftinginterpreters.com/scanning.html)

## 状态

- [x] 已读（笔记整理）

---

## 一句话

Part II 第一段代码：实现 jlox 扫描器（Scanner）——编译/解释流水线的第一站（[ch02 §2.1.1](../../part01_welcome/chapter02_map-of-the-territory/README.md)）。

---

## 专项笔记（一小节一文件）

| 小节 | 主题 | 阅读 |
|------|------|------|
| — | 本章定位 | [00-overview.md](./00-overview.md) |
| §4.1 | 解释器框架（The Interpreter Framework） | [01-the-interpreter-framework.md](./01-the-interpreter-framework.md) |
| §4.2 | 词素与词法单元（Lexemes and Tokens） | [02-lexemes-and-tokens.md](./02-lexemes-and-tokens.md) |
| §4.3 | 正则语言与表达式（Regular Languages and Expressions） | [03-regular-languages-and-expressions.md](./03-regular-languages-and-expressions.md) |
| §4.4 | Scanner 类（The Scanner Class） | [04-the-scanner-class.md](./04-the-scanner-class.md) |
| §4.5 | 识别词素（Recognizing Lexemes） | [05-recognizing-lexemes.md](./05-recognizing-lexemes.md) |
| §4.6 | 更长的词素（Longer Lexemes） | [06-longer-lexemes.md](./06-longer-lexemes.md) |
| §4.7 | 保留字与标识符（Reserved Words and Identifiers） | [07-reserved-words-and-identifiers.md](./07-reserved-words-and-identifiers.md) |
| ·9 | 流水线位置与后续章节 | [08-pipeline.md](./08-pipeline.md) |
| — | 速记 · 自测 · 进度 |

---

## 逻辑脉络


---

## 速记

## 本章速记

```text
§4.1  main: 0→REPL · 1→runFile · >1→exit(64) · 有错 exit(65)
       runFile / REPL · hadError · error(line) → report · 有错不执行残缺代码
§4.2  Lexeme = 纯文本片段 · Token = type + lexeme + literal + line · Parser 只吃 Token
§4.3  Scanner 四步循环 · 词法=正则 · 嵌套=CFG/Parser · Flex vs 手写
§4.4  start/current/line · substring(start,current) · peek vs advance · List+EOF
§4.6  // 注释 · 字符串 · double · peekNext
§4.7  maximal munch · Map 查保留字
```

---

---

## 读后下一步

| 章 | 目录 | 内容 |
|:--:|------|------|
| **5** | [chapter05 · Representing Code](../chapter05_representing-code/) | 定义 **AST 节点**（树的数据结构） |
| **6** | Parsing | Token → AST |

写 parser 前务必完成 ch5 的表达式/语句节点类（附录 II 有生成代码参考）。

---

---

## 自测 / Challenges（可选）

- [ ] 说明 jlox 退出码 **64** / **65** 与 REPL 下 `hadError` 重置策略。
- [ ] 画 `Scanner` 在 `"var x = 1.5; // hi\n"` 上的 `start/current/line` 变化。
- [ ] 举 2 个「正则能处理」和 2 个「必须 Parser」的 Lox 结构（见 §4.3）。
- [ ] 说明 **maximal munch** 若不用，关键字 `or` 与标识符 `orchid` 会如何冲突。
- [ ] 对比 **03**《自制编译器》：JavaCC 生成扫描器 vs 本书手写——各牺牲/得到什么。

---

---

## 阅读进度

- [x] §4.1～§4.7 结构梳理（本章笔记）
- [ ] 跟书实现 / 对照 GitHub `jlox` 源码
- [ ] 本章 Challenges

