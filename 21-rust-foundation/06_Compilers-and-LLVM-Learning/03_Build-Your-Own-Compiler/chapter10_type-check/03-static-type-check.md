# 第 10 章 · 语义分析（2）静态类型检查 · §3 静态类型检查

← [本章目录](./README.md) · 上一节：[02-expression-validity.md](./02-expression-validity.md) · 下一节：---

## 类型限制的检查

各运算对 **操作数类型** 有 **硬限制**：

| 非法（概念） | 原因 |
|--------------|------|
| **struct + struct** | 无定义算术 |
| **指针 * 整数** 以外乱配 | 仅规定组合合法 |
| 指针 + 指针（非相减场景） | 等 |

**`TypeChecker`**（Visitor）遍历节点 — 查 **运算表** / `switch(op)` 验证 **左右 Type**。

---

## 隐式类型转换 · CastNode

允许但不一致 → 在 AST **插入 `CastNode`** — **编译期** 转换，非 IR 运行时。

```text
char c; … c + 1 …
→ 可能 Insert CastNode(c, int) 再相加
```

**下游**：ch11 IR 生成 **按 CastNode 发扩展/截断指令**。

---

## 整型提升（Integral Promotion）

**较小整型** 在运算前 **提升** 为 `int`（等规则）：

```text
char + char  →  int + int（概念）
```

C 规则 — Rust **无** 自动 char→int 提升。

---

## 寻常算术转换（Usual Arithmetic Conversion）

**二元运算** 左右类型不同但可兼容 → **统一为公共类型**（优先级规则：如 `long` > `int` …）。

```text
int + long  →  long + long
float + double → double + double
```

TypeChecker **插入双侧 CastNode** 或 **标记结果类型**。

---

## 类型推导方向 · 自下而上

**Visitor 自叶子向上**：

```text
IntegerLiteral → 已知 int 类型
VariableNode   → 已消解定义的类型
       ↓
UnaryOp / BinaryOp → 由子节点 Type + 规则 推导父 Type
       ↓
AssignNode → 检查可赋值 + 可能 Cast 右值
```

| 方向 | 原因 |
|------|------|
| **自底向上** | 子表达式类型 **先确定**，父才能检查 |

与 [ch8 AST 构建](../chapter08_build-ast/00-overview.md) **同向** — 构建 bottom-up，检查亦 bottom-up。

---

## 与 ch11

语义正确的 AST（含 **CastNode**、每 Expr **结果 Type**）→ **`IRGenerator`** [ch11](../chapter11_ir/)（待建）。

---

## 自测

- [ ] CastNode 何时插入
- [ ] 整型提升 vs 寻常算术转换 各一句话
- [ ] TypeChecker 为何从叶子向上
