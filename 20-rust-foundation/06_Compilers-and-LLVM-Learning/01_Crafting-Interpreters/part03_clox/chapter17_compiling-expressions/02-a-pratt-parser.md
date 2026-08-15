# 第 17 章 · Compiling Expressions（编译表达式） · 普拉特解析器（A Pratt Parser）

← [本章目录](./README.md) · 上一节：[01-single-pass-compilation.md](./01-single-pass-compilation.md) · 下一节：[03-emitting-bytecode.md](./03-emitting-bytecode.md)

---

Vaughan Pratt 的 **自顶向下运算符优先级解析**（Top-Down Operator Precedence）。

**核心结构**（C 中函数指针分发表）：

```c
typedef void (*ParseFn)(bool canAssign);

ParseRule rules[] = {
  // TokenType → { prefixFn, infixFn, precedence }
};
```

| 概念 | 说明 |
|------|------|
| **prefix 解析函数** | 处理 **`−`**、**`(`**、数字等 **前缀** 位置 |
| **infix 解析函数** | 处理 **`+`**、**`*`** 等 **中缀** 位置 |
| **precedence** | 数值越大绑定越紧；控制 **结合性**（左结合：先 parse 高优先级右侧） |

**入口**：

```c
void expression() {
  parsePrecedence(PREC_ASSIGNMENT);
}
```

**`parsePrecedence(minPrec)`** 循环：

1. 读 prefix rule → 调用 prefix 函数（已消费首 token）。
2. while 下一 token 的 infix 优先级 ≥ `minPrec` → 调用 infix 函数。

**对照 jlox [ch6](../../part02_jlox/chapter06_parsing-expressions/README.md)**：递归下降每层一个函数；Pratt 用 **表驱动 + 优先级** 统一处理运算符。

---
