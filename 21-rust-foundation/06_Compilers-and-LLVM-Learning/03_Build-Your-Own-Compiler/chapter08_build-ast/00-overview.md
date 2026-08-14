# 第 8 章 · 抽象语法树的生成 · 本章定位

← [本章目录](./README.md) · 上一章：[ch7 action 和 AST](../chapter07_javacc-ast/README.md) · 下一章：[ch9 语义分析（1）引用的消解](../chapter09_name-resolution/README.md) · 下一节：[01-expr-ast.md](./01-expr-ast.md)

---

```text
ch7  action 机制 + Node 类模型
  ↓
ch8  在 .jj 里为 C♭ 全文 new 节点  ← 本章
  ↓
ch9  遍历 AST 做名字/类型语义
```

**构建顺序**：JavaCC **自底向上**（末端规则先执行 action）→ 笔记 **小单位 → 大单位**：表达式 → 语句 → 声明 → 启动 Parser。

**产出**：`parse()` 返回 **`AST` 根** — [ch2 `compile`](../chapter02_cflat-cbc/03-compiler-control-flow.md) 第一步完整闭环。
