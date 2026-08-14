# §2.1.4 静态分析（Static Analysis）
← [§2.1 hub](./01-the-parts-of-a-language.md) · 上一节 · [§2.1.3](./01-3-intermediate-representations.md) · 下一节 · [§2.1.5](./01-5-optimization.md)

---


> **原书编号 §2.1.3** · 流水线位置：Parsing **之后**、Lowering 到 IR **之前**（概念上在 §2.1.3 IR 降维之前）。

| 项目 | 说明 |
|------|------|
| **位置** | 前端末段 / **middle end** 入口 |
| **输入** | **AST**（只有语法形状） |
| **输出** | 带语义信息的 AST、**符号表**，或更低层 IR |
| **核心动作** | **Binding / 类型检查 / 作用域解析** |
| **不做** | 生成机器码、运行程序 |

**Parsing 之后仍不知道什么**

```lox
print a + b;
```

Parser 只得到 `Print(Binary(+, Var(a), Var(b)))` — **`a`、`b` 是谁？在哪定义？什么类型？** 静态分析回答这些。

---

#### 例子 1 · 名字绑定（Binding / Resolution）

**源码：**

```lox
var x = 1;
{
  print x;
}
```

**分析结果（概念）：**

```text
符号表:
  x → 全局槽 #0  （或当前环境 depth=0, slot=0）

AST 节点 Var("x") 附加属性:
  resolved: Global #0
  或 distance: 1, slot: 0   （jlox ch11 Resolver）
```

**jlox**：`Resolver` 遍历 AST，给每个 `Var` 写入 **upvalue distance** → 运行时不再按名字查找。

→ [jlox ch11 Resolving and Binding](../../part02_jlox/chapter11_resolving-and-binding/README.md)

---

#### 例子 2 · 作用域错误（静态报错）

**源码：**

```lox
{
  print inner;
  var inner = 1;
}
```

| 阶段 | 结果 |
|------|------|
| Scanner / Parser | ✅ 语法合法 |
| **静态分析** | ❌ `inner` 在声明前使用 — **语义错误** |

**Rust 对照：**

```rust
let x = y;  // ❌ 编译期：cannot find value `y`
let y = 1;
```

---

#### 例子 3 · 类型检查（静态类型语言）

**Lox** — **动态类型**，静态分析阶段**不做**完整类型检查；`"hi" + 3` AST 合法，**运行时才报错**。

**Rust** — **静态类型**，同一阶段拒绝：

```rust
let s: &str = "hi";
let n: u32 = s + 3;   // ❌ 编译期 type mismatch
```

| | **Lox / Python 风格** | **Rust / C 风格** |
|---|----------------------|-------------------|
| 类型 | 运行时查 | **编译期**查 |
| 静态分析重点 | 绑定、闭包捕获 | 绑定 + **类型** + 借用（MIR 上 borrowck） |

---

#### 例子 4 · 信息存哪

| 方式 | 例子 |
|------|------|
| **AST 节点附加字段** | `Var.resolved_slot` · `Expr.ty` |
| **符号表（side table）** | `HashMap<Ident, Decl>` |
| **换成新 IR** | AST → HIR/MIR（Rust）— 语义更直白 |

```text
Parser:     AST（空语义字段）
Resolver:   AST + distance/slot
Typeck:     AST/HIR + 每个 expr 的 Ty
Lowering:   → §2.1.3 IR
```

---

#### 例子 5 · 前端 / middle end / 后端（原书分界）

```text
Scan · Parse · Static Analysis     ← 前端（+ 部分 middle end）
IR · Optimization                  ← middle end
Code Gen · VM                      ← 后端
Runtime                            ← 运行期
```

**「编译之山」山顶**：静态分析完成时，编译器对程序**语义**有完整鸟瞰，再开始「下山」降 IR、优化、出码。

---

#### 例子 6 · 易错边界（自测用）

**1. 静态分析 ≠ 优化** — 前者**理解**程序；后者在保持语义下**改写**程序。

**2. jlox 树遍历仍要做 Resolver** — 静态分析不必等 IR；可在 AST 上完成。

**3. `a + b` 类型兼容** — Lox 运行期；Rust `i32 + &str` 编译期挂。

---

**一句话**：静态分析给 AST **填语义** — 谁是谁、能否相加、变量在哪；**仍属编译期**，不产生可执行文件。

**本书对应**：jlox **ch11 Resolver** · clox 在编译期做 **Upvalue 解析**（ch22 等）

**本仓库**：Rust borrowck → [Nomicon 03 Lifetime](../../../../04-Rust-Nomicon/03_Lifetime_Variance/README.md) · BYOC **`type` 包**

→ 下一节：[§2.1.5](./01-5-optimization.md)

---
