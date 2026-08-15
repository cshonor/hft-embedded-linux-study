# 第 4 章 · 词法分析 · 本章定位

← [本章目录](./README.md) · 上一章：[ch4 词法分析](../chapter04_lexical/README.md) · 下一章：[ch5 基于 JavaCC 的解析器的描述](../chapter05_javacc-parser/README.md) · 下一节：[01-javacc-regex.md](./01-javacc-regex.md)

---

```text
ch3  token / JavaCC 概要
  ↓
ch4  .jj 里写扫描规则  ← 本章
  ↓
ch5  .jj 里写语法产生式
```

| 对比 | [CI ch4](../../../01_Crafting-Interpreters/part02_jlox/chapter04_scanning/) | 本章 |
|------|-------------------------------------------------------------------------------|------|
| 实现 | 手写 `Scanner` | **声明式正则** + JavaCC 生成 |

**产出**：C♭ 源文件 → **token 流** — Parser（ch6）的输入。
