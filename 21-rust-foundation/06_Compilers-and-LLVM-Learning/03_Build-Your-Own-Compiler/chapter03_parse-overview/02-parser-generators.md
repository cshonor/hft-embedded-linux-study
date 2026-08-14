# 第 3 章 · 语法分析的概要 · §2 解析器生成器

← [本章目录](./README.md) · 上一节：[01-analysis-phases-and-tokens.md](./01-analysis-phases-and-tokens.md) · 下一节：[03-javacc-overview.md](./03-javacc-overview.md)

---

## 为何需要生成器

手写 Scanner/Parser：

| 问题 | 说明 |
|------|------|
| **重复** | 大量相似的状态/递归代码 |
| **易错** | 文法稍改，手工同步多处 |
| **难维护** | 语言演进成本高 |

**生成器思路**：开发者写 **语法规则**（声明式）→ 工具 **自动生成** 扫描/解析 Java 代码。

```text
  文法描述 (.jj / .y)
        ↓  javacc / yacc
  生成的 .java
        ↓  javac
  Scanner + Parser 类
```

与 [EaC ch2 有限自动机→扫描器生成](../../../02_Compiler-Principles/chapter02_scanners/) 同族思想。

---

## LL vs LALR 两大阵营

| 类型 | 代表 | 特点 |
|------|------|------|
| **LALR** | UNIX **yacc** / **Bison** | 文法覆盖 **广**；工业 C 编译器常用 |
| **LL** | **JavaCC**、ANTLR（LL(*)） | 结构 **更简单**；生成代码 **更易读** |

```text
        解析器生成器
       /            \
    LALR              LL
  广、表驱动        递归下降风格
  yacc/bison        JavaCC（本书）
```

**本书选择**：C♭ 文法不需 yacc 级复杂度；**可读性 + Java 生态** 优先。

---

## 为何选 JavaCC

| 理由 | 说明 |
|------|------|
| **功能足够** | 覆盖 C♭ 词法/语法需求 |
| **无额外运行时库** | 生成 plain Java — 部署简单 |
| **成熟** | 社区与文档可用 |
| **可读生成码** | 便于教学 **对照 .jj 与输出** |

**对比 CI**：jlox **手写** scanner/parser — 更透明；cbc **JavaCC** — 更省重复劳动。见 [CI ch5 JavaCC 对照](../../../01_Crafting-Interpreters/part02_jlox/chapter05_representing-code/01-context-free-grammars.md)。

**Rust 生态**：`lalrpop`（LALR）、`logos`+手写 parse 等 — 思想相同，工具不同。

---

## 自测

- [ ] 生成器解决什么问题
- [ ] LL 与 LALR 各一句话
- [ ] 本书选 JavaCC 的四条理由
