# 第 2 章 · 扫描 · §1 扫描器的核心任务

← [本章目录](./README.md) · 上一节：[00-overview.md](./00-overview.md) · 下一节：[02-regular-expressions.md](./02-regular-expressions.md)

---

**扫描器（Scanner / Lexer / 词法分析器）** 是编译器处理输入的**第一个阶段**。

---

## 做什么

| 步骤 | 说明 |
|------|------|
| **逐字符读取** | 扫描源代码字符流 |
| **组合成「字」** | 变量名、数字、符号等符合语言规则的词法单元 |
| **确定词性** | 每个字的**语法范畴**（标识符、关键字、运算符、字面量…） |
| **过滤噪声** | 丢弃空格、换行、注释（对后续阶段不可见） |

---

## 输出

**Token 流** — 交给 **语法分析器（Parser，ch3）**。

```text
let x: u64 = 10;
→ [Let] [Identifier("x")] [Colon] [Type("u64")] [Equal] [Integer(10)] [Semicolon]
```

（具体 Token 划分因语言而异。）

---

## 比喻与位置

- 扫描器 = 编译器的**「眼睛」**。
- 在 [ch1 翻译流程](../chapter01_overview/04-translation-pipeline-example.md) 中属于 **「理解输入 · 词法」** 层。
- 在 [CI 流水线](../../../01_Crafting-Interpreters/part01_welcome/chapter02_map-of-the-territory/04-rust-hft-编译流水线对照.md) 中 = **Scanning / Lexing → Tokens**。

---

## 与 CI 手写扫描的对比

| | **橡书 ch2（理论 + 自动生成）** | **CI jlox ch4 / clox ch16** |
|---|-------------------------------|------------------------------|
| 重点 | RE → DFA → 最小化 → 代码生成 | 直接写 `Scanner` 类 / C 函数 |
| 工具链 | lex/flex 类生成器背后的数学 | 理解 Token 与错误恢复 |

两者互补：CI 动手；橡书解释「生成器为什么能工作」。
