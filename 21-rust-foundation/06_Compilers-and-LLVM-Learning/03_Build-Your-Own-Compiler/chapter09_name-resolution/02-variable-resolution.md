# 第 9 章 · 语义分析（1）引用的消解 · §2 变量引用的消解

← [本章目录](./README.md) · 上一节：[01-semantic-overview-visitor.md](./01-semantic-overview-visitor.md) · 下一节：[03-type-name-resolution.md](./03-type-name-resolution.md)

---

## 核心目的

C♭ 有 **作用域** — 同名可能指 **全局 / 静态 / 局部** 不同定义。

| 解析前 | 解析后 |
|--------|--------|
| **`VariableNode`** 仅 **名字** | 绑定到 **`DefinedVariable`**（或等价定义对象） |

**引用消解** = 消除「这个名字到底是谁」的不确定性。

→ [ch8 `VariableNode`](../chapter08_build-ast/01-expr-ast.md)

---

## LocalResolver 与 Scope 树

**`LocalResolver`** 实现变量消解 — 用 **Scope 类层次** 记录嵌套：

| 类 | 角色 |
|----|------|
| **`Scope`** | 抽象作用域 |
| **`ToplevelScope`** | **文件/全局** 顶层 |
| **`LocalScope`** | **块内** 局部 |

**构建**：用 **栈 Stack** 随 AST 进入/离开 **`BlockNode`** **压入/弹出** Scope — 形成 **Scope 树**（嵌套关系）。

```text
ToplevelScope
  └── LocalScope (main 块)
        └── LocalScope (if 内层块)
```

---

## 查找机制

引用变量时（访问 **`VariableNode`**）：

```text
1. 从 **当前最内层** Scope 查符号表
2. 未找到 → **沿 Scope 树向上**（父 Scope）
3. 直到 **ToplevelScope**
4. 找到 → 将 **定义** 关联到 VariableNode
5. 顶层仍无 → **未定义错误**（带 location()）
```

| 原则 | 说明 |
|------|------|
| **最近嵌套优先** | 内层遮蔽外层 — 与 C 一致 |
| **向上追溯** | 链式父指针或栈顶向下搜 |

**Visitor 遍历顺序**：进入 Block **先** push 声明 **再** 访问子节点 **最后** pop — 保证引用时 Scope 正确。

---

## 与 ch8 BlockNode

[BlockNode](../chapter08_build-ast/02-stmt-ast.md) 携带 **局部声明列表** — `LocalResolver` 进块时 **注册** 这些名字到 `LocalScope`。

---

## 自测

- [ ] 为何不能只看变量名字符串
- [ ] ToplevelScope vs LocalScope
- [ ] 查找失败时在哪一层报错
