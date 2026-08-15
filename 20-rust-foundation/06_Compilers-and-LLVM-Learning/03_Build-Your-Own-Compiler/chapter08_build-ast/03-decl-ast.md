# 第 8 章 · 抽象语法树的生成 · §3 声明的抽象语法树

← [本章目录](./README.md) · 上一节：[02-stmt-ast.md](./02-stmt-ast.md) · 下一节：[04-parser-startup.md](./04-parser-startup.md)

---

## 变量声明 · 多变量一条语句

C/C♭ 风格：`int x, y, z;` — **一条规则，多个变量**。

| 产出 | 说明 |
|------|------|
| **`List<DefinedVariable>`** | 每个变量一个 **`DefinedVariable`** |

```text
defvars action:
  类型 + ( 名 + 后置修饰 )*
  → 循环生成多个 DefinedVariable，加入 list
```

→ [ch6 后置修饰](../chapter06_parsing/01-definitions.md) 在 **每个名字** 上分别挂 `*`、`[]`。

---

## 函数定义 · `DefinedFunction`

| 字段（概念） | 内容 |
|--------------|------|
| 返回类型 | **`TypeRef` / `Type`** |
| 函数名 | 标识符 |
| 参数列表 | **`List<Parameter>`** |
| 函数体 | **`BlockNode`** |

```text
int foo(int a) { … }  →  DefinedFunction(…)
```

顶层 **`defun`** 产生式 action 构造。

---

## 程序根 · `AST`

**`compilation_unit`** → 全文件 **根节点 **`AST`**：

| 合并内容 | 来源 |
|----------|------|
| 本文件定义 | `top_defs` — 函数/变量/struct/typedef |
| 外部导入 | `import_stmts` 解析结果 |

```text
AST
  ├── imports / 外部声明
  └── definitions: DefinedFunction, DefinedVariable, …
```

**一棵树的顶** — `dump()` / ch9 以后 Pass 的 **入口**。

---

## 外部符号 · `import`

```c
import stdio;
```

| 步骤 | 说明 |
|------|------|
| 解析 import | action 触发 |
| **读库文件** | 如 **`stdio.hb`**（C♭ 库描述） |
| 合并声明 | 库中函数/类型 **加入程序声明列表** |

无 C 预处理器 — **编译期显式加载** 替代 `#include` 文本展开。

→ 链接阶段仍与 **实际目标/库** 对应（第4部分 ch19）。

---

## struct / typedef（概念）

**`defstruct`** · **`typedef`** — 对应 **`TypeDefinition`** 等节点（[ch7 基类](../chapter07_javacc-ast/02-ast-and-nodes.md)）— 细节在 cbc `ast` 包与 `.jj` 对照阅读。

---

## 自测

- [ ] 一条 `int x,y` 对应几个 DefinedVariable
- [ ] AST 根合并哪两类内容
- [ ] import 与 `#include` 机制差异（本书）
