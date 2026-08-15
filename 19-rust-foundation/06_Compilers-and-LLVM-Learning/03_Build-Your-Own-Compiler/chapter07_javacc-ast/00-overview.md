# 第 7 章 · JavaCC 的 action 和抽象语法树 · 本章定位

← [本章目录](./README.md) · 上一章：[ch6 语法分析](../chapter06_parsing/README.md) · 下一章：[ch8 抽象语法树的生成](../chapter08_build-ast/README.md) · 下一节：[01-javacc-actions.md](./01-javacc-actions.md)

---

```text
第1部分  ch3～6   文法（无树 / 仅 parse）
  ↓
第2部分  ch7     action + Node 模型  ← 本章
  ↓
ch8      各类节点在 .jj 里怎么 new
```

| [ch2 `compile`](../chapter02_cflat-cbc/03-compiler-control-flow.md) | 本章 |
|-----------------------------------------------------------------------|------|
| parse → **AST** | **parse 同时构造** AST — 不再只是「匹配成功」 |

**工具链**：`.jj` 规则 + `{}` action → **`ast` 包** 节点树 → ch9 语义 · ch11 IR。
