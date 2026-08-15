# §2.1.2 解析（Parsing）
← [§2.1 hub](./01-the-parts-of-a-language.md) · 上一节 · [§2.1.1](./01-1-scanning-lexing.md) · 下一节 · [§2.1.3](./01-3-intermediate-representations.md)

---


| 项目 | 说明 |
|------|------|
| **位置** | 流水线**第二步**（接在 Scanning 之后） |
| **输入** | 扁平 **Token 序列** |
| **输出** | **AST**（抽象语法树），反映嵌套层级 |
| **核心动作** | 按**语法规则（文法）**消费 Token → 建树 |
| **不做** | 类型检查、作用域绑定、优化（属语义分析） |

**Token 流 vs AST**

| | **Token 流** | **AST** |
|---|--------------|---------|
| 形状 | **线性**列表 | **树**（父节点 = 语法结构，子节点 = 组成部分） |
| 信息 | 只有「下一个是什么符号」 | 表达「谁包含谁、谁先算谁」 |
| 谁产出 | Scanner（§2.1.1） | **Parser** |
| 谁消费 | Parser | 解释器 / 编译器后端 / 语义分析 |

```text
Token 流（扁平）  →  Parser（按文法匹配）  →  AST（树）
```

---

#### 例子 1 · Lox 变量声明（Token → 语句树）

**接 §2.1.1 的 Token 流：**

```text
VAR · IDENTIFIER("price") · EQUAL · NUMBER(19.99) · SEMICOLON
```

**Parser 产出的 AST（概念结构）：**

```text
Stmt.Var
├── name: "price"
└── initializer: Expr.Literal(19.99)
```

**对比**：Token 流是 5 个平铺符号；AST 说清「这是一条 `var` 声明，名字叫 price，初值是数字 19.99」。

---

#### 例子 2 · 表达式优先级（Parser 的核心价值）

**源码：**

```lox
1 + 2 * 3
```

**Token 流（7 个）：**

```text
NUMBER(1) · PLUS · NUMBER(2) · STAR · NUMBER(3) · EOF
```

**错误理解**（若按 Token 顺序线性读）：「从左到右依次加/乘」→ 会得到 `(1+2)*3 = 9`。

**正确 AST**（Parser 按优先级建树）：

```text
        Binary(+)
       /         \
  Literal(1)   Binary(*)
              /         \
        Literal(2)   Literal(3)
```

**AstPrinter 风格输出**（jlox ch5）：

```text
(+ 1 (* 2 3))    →  先算 2*3，再加 1  →  7
```

**规则**：优先级高的运算符在树上**更深**（更靠近叶子）。Lox 分层（低→高）：`equality` → `comparison` → `term(+/-)` → `factor(*/)` → `unary` → `primary`。

→ [ch6 §6.1 歧义与优先级](../../part02_jlox/chapter06_parsing-expressions/01-ambiguity-and-the-parsing-game.md)

---

#### 例子 3 · 左结合（同级运算符）

**源码：**

```lox
10 - 3 - 1
```

**AST（左结合）：**

```text
      Binary(-)
     /         \
 Binary(-)   Literal(1)
 /         \
Lit(10)   Lit(3)

→ ((10 - 3) - 1) = 6
```

Parser 典型写法（递归下降）：`while (match(MINUS))` 不断把左子「长高」→ 自然左结合。

→ [ch6 §6.2 递归下降](../../part02_jlox/chapter06_parsing-expressions/02-recursive-descent-parsing.md)

---

#### 例子 4 · Rust 对照（`let` + 类型 + 块）

**源码：**

```rust
let x: u64 = 10;
```

**Token 流（概念）：**

```text
KW_LET · IDENT("x") · COLON · IDENT("u64") · EQ · LIT_INT(10) · SEMI
```

**AST 片段（概念，类似 `rustc` HIR 之前的语法树）：**

```text
StmtLet
├── pat: PatIdent("x")
├── ty:  TyPath("u64")      ← Parser 只认「类型语法形状」
└── init: ExprLit(10)
```

**Parser 仍不管**：`u64` 是否已定义、`10` 是否溢出 — 那是**语义分析 / 类型检查**（Rust 的 borrowck 更在后面）。

---

#### 例子 5 · 分组改变结构

**源码：**

```lox
(1 + 2) * 3
```

**Token 流：**

```text
LEFT_PAREN · NUMBER(1) · PLUS · NUMBER(2) · RIGHT_PAREN · STAR · NUMBER(3)
```

**AST：**

```text
      Binary(*)
     /         \
 Binary(+)   Literal(3)
 /       \
Lit(1)  Lit(2)

→ (1+2)*3 = 9   （与 例子 2 的 1+2*3 不同）
```

括号在 Parser 里通常由 **`primary` 规则**处理：遇到 `(` → 递归调用 `expression()` → 期望 `)`。

---

#### 例子 6 · Parser 报什么错（词法过关、语法不过）

| 源码 | Scanner | Parser |
|------|---------|--------|
| `var x = ;` | ✅ Token 正常 | ❌ `=` 后期望表达式，遇到 `;` |
| `if (cond) {` 缺 `}` | ✅ | ❌ 块未闭合 |
| `(1 + 2` | ✅ | ❌ 缺 `)` |
| `@foo` | ❌ 非法字符（扫描阶段） | — |

Parser 用 **`match` / `check` / `advance`** 同步 Token 流；期望的类型对不上 → **语法错误**（jlox §6.3 精确定位行号）。

→ [ch6 §6.3 语法错误](../../part02_jlox/chapter06_parsing-expressions/03-syntax-errors.md)

---

#### 例子 7 · 递归下降在脑中的调用栈

**源码：** `1 + 2 * 3`

```text
expression()
  └─ equality()
       └─ comparison()
            └─ term()          ← 看到 1 + …
                 ├─ factor() → Literal(1)
                 ├─ match(PLUS) ✓
                 └─ term 循环结束，但 + 右操作数要更高优先级层：
                      factor() → 进入 term 内再调 factor
                           ├─ factor() → Literal(2)
                           ├─ match(STAR) ✓
                           └─ factor() → Literal(3)
                 → 组装 Binary(+, 1, Binary(*, 2, 3))
```

**一句话**：低优先级函数包外层，内部调用高优先级函数 → 树自然正确。

---

#### 例子 8 · 易错边界（自测用）

**1. Parser 不检查类型**

```lox
var x = "hello" + 3;   // AST 合法；运行时才可能报错
```

**2. 同一串 Token，不同文法 → 不同树**

```lox
- 1 - 2     // 一元负号 vs 二元减，由 unary / term 分层消歧
```

**3. 空文件**

```text
Token: EOF only  →  AST: 空程序（零语句）
```

---

**一句话**：Parser 做「**搭骨架**」— 把 Token 串按文法拼成树；**不管**类型对不对、变量有没有定义（那是语义分析 / 运行时）。

**本书对应**：Part II **ch5**（AST 类型 + Visitor）· **ch6**（递归下降 Parser）· Part III clox **ch17** 起（编译表达式为字节码，内部另有编译器前端）。

**本仓库**：深入 [ch5 §5.2 实现语法树](../../part02_jlox/chapter05_representing-code/02-implementing-syntax-trees.md) · [ch6 §6.2 递归下降](../../part02_jlox/chapter06_parsing-expressions/02-recursive-descent-parsing.md) · **03** BYOC 语法分析章节

→ 下一节：[§2.1.3](./01-3-intermediate-representations.md)

---
