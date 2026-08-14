# 第 6 章 · Parsing Expressions（解析表达式） · §6.2 递归下降解析（Recursive Descent Parsing）

← [本章目录](./README.md) · 上一节：[01-ambiguity-and-the-parsing-game.md](./01-ambiguity-and-the-parsing-game.md) · 下一节：[03-syntax-errors.md](./03-syntax-errors.md)

---

### 为何选递归下降

| 特点 | 说明 |
|------|------|
| **自顶向下** | 从起始符号 `expression` 往下拆 |
| **手写友好** | 每条文法规则 ≈ 一个 Java 方法 |
| **速度快、健壮** | 工业界常用（许多生产编译器前端） |
| **错误处理** | 可精细控制（§6.3） |

### 规则 → 代码

```text
equality   →  equality()   方法
comparison →  comparison() 方法
term       →  term()       方法
…
```

### Token 流辅助方法

| 方法 | 作用 |
|------|------|
| **`match(type, …)`** | 若当前 Token 类型匹配 → **消耗**并返回 true |
| **`check(type)`** | **只看**当前类型，不消耗 |
| **`advance()`** | 读下一个 Token |
| **`previous()`** | 刚消耗的那个 Token（常取 literal） |

### 构建 `Expr.Binary` 与左结合

- 二元层（如 `term` 处理 `+` `-`）典型模式：

```text
Expr expr = factor();           // 左操作数
while (match(PLUS, MINUS)) {
  Token op = previous();
  Expr right = factor();
  expr = new Binary(expr, op, right);  // 左结合：新节点以旧 expr 为左子
}
return expr;
```

- **`while` + 左子不断「长高」** → 自然实现**左结合**（`1 - 2 - 3`）。

**验证**：用 ch5 **`AstPrinter`** 打印 `(1 - 2 - 3)` 的树形。

---
