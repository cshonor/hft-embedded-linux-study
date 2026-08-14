# 第 3 章 · 语法分析的概要 · §3 JavaCC 的概要

← [本章目录](./README.md) · 上一节：[02-parser-generators.md](./02-parser-generators.md) · 下一节：---

## 语法描述文件 `.jj`

JavaCC 规则写在 **`.jj`** 文件中，典型结构：

```text
options { … }

PARSER_BEGIN(ParserClassName)
// Java 代码：解析器类的成员、入口方法等
PARSER_END(ParserClassName)

// TOKEN 定义、词法规则
// 语法产生式
```

| 区块 | 作用 |
|------|------|
| **`options`** | 生成器行为（如 Unicode、调试） |
| **`PARSER_BEGIN` / `PARSER_END`** | 包裹 **自定义 Java** — 生成类的主干 |
| **规则区** | **TOKEN**（扫描）+ **产生式**（解析） |

→ ch5 专讲 EBNF 与 `.jj` 写法；ch6 完整 C♭ 文法。

---

## 运行流程

```bash
javacc grammar.jj      # .jj → 多个 .java（Parser、TokenManager…）
javac *.java           # 标准 Java 编译
java …                 # 调用生成的 Parser
```

```text
  .jj  ──javacc──→  XxxParser.java, Token.java, …
                         ↓ javac
                    可链接进 cbc
```

**与 cbc**：`parser` 包中可见生成类 + 手写 **语义动作**（ch7）。

---

## 中文与 Unicode

默认 JavaCC 可能 **无法** 正确处理中文标识符/注释。

| 设置 | 说明 |
|------|------|
| **`options { UNICODE_INPUT = true; }`** | `.jj` 内启用 Unicode 输入 |
| **Reader 编码** | 实例化 Parser 时对 **InputStream/Reader** 指定 **`UTF-8`**（等） |

```text
// 概念：启动解析时
new Parser(new InputStreamReader(stream, "UTF-8"));
```

**C♭ 源码** 通常 ASCII 即可；选项为 **扩展/注释中文** 预留 — 与「源文件编码统一 UTF-8」工程习惯一致。

---

## 与 ch4～5 衔接

| 章 | 内容 |
|----|------|
| **ch4** | `.jj` 里 **TOKEN**、各类单词的扫描规则 |
| **ch5** | **EBNF 产生式**、lookahead、歧义 |
| **ch6** | C♭ 完整 **定义/语句/表达式** 解析 |

---

## 自测

- [ ] `.jj` 三块结构（options / PARSER_BEGIN/END / 规则）
- [ ] `javacc` 与 `javac` 各做什么
- [ ] 中文支持需哪两处配置
