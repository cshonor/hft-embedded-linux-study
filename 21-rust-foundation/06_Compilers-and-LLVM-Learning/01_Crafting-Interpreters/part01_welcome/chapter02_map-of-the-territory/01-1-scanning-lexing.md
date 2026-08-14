# §2.1.1 扫描 / 词法分析（Scanning / Lexing）
← [§2.1 hub](./01-the-parts-of-a-language.md) · 上一节 · [00-overview](./00-overview.md) · 下一节 · [§2.1.2](./01-2-parsing.md)

---


| 项目 | 说明 |
|------|------|
| **位置** | 流水线**第一步** |
| **输入** | 源代码**字符流** |
| **输出** | **Token**（词法单元）序列 |
| **核心动作** | 逐字符读入 → 识别**词素（Lexeme）** → 打包成带类型的 Token |
| **丢弃** | 空白、换行、注释（不进入 Token 流） |

**Lexeme vs Token**

| | **Lexeme** | **Token** |
|---|------------|-----------|
| 是什么 | 源码里的**文本片段** | 扫描器产出的**结构化对象** |
| 例子 | 字符 `123` | `NUMBER(123.0)` + 行号 |
| 谁用 | 人眼在源码里看到 | **Parser** 读 Token 流建 AST |

```text
源码字符  →  Lexeme（文本）  →  Token（类型 + 字面量 + 位置）
```

---

#### 例子 1 · Lox 一行语句（本书语言）

**源码：**

```lox
var price = 19.99; // unit price
```

**扫描后 Token 流**（空白与注释已丢弃）：

```text
VAR          "var"        @1
IDENTIFIER   "price"      @1
EQUAL        "="          @1
NUMBER       19.99        @1
SEMICOLON    ";"          @1
EOF                       @2
```

**逐字符怎么走**（简化）：

```text
v a r   → 关键字 VAR
p r i c e → IDENTIFIER（保留字表里没有 price）
=       → EQUAL
1 9 . 9 9 → NUMBER（读到非数字/非小数点停止）
;       → SEMICOLON
/ / ... → 整行注释，跳过，不产生 Token
```

**多字符运算符**（不能拆成两个 Token）：

```lox
a != b
```

```text
IDENTIFIER "a"
BANG_EQUAL "!="    ← 一个 Token，不是 BANG + EQUAL
IDENTIFIER "b"
```

→ 实现细节：[jlox ch4 · 识别词素](../../part02_jlox/chapter04_scanning/05-recognizing-lexemes.md) · [保留字与标识符](../../part02_jlox/chapter04_scanning/07-reserved-words-and-identifiers.md)

---

#### 例子 2 · Rust 一行（对照 rustc 前端）

**源码：**

```rust
let x: u64 = 10; // counter
```

**等价 Token 流（概念上，与 `rustc` 内部一致）：**

```text
KW_LET       "let"
IDENT        "x"
COLON        ":"
IDENT        "u64"        ← 类型名在词法阶段只是标识符
EQ           "="
LIT_INT      10           ← 字面量已带数值/类型信息
SEMI         ";"
EOF
```

**与 Lox 的相同点**：都是**线性 Token 列表**，Parser 下一步才关心 `let x: u64 = 10` 的嵌套结构。  
**不同点**：Rust 词法更复杂（生命周期 `'a`、raw string `r#"..."#`、字节串 `b"..."` 等）。

---

#### 例子 3 · 字符串与数字（词素边界）

**Lox：**

```lox
print "hi\n";
```

| 阶段 | 内容 |
|------|------|
| 源码字符 | `"` `h` `i` `\` `n` `"` `;` |
| Lexeme | `"hi\n"`（含转义，引号不算进字面量） |
| Token | `STRING` · literal = 运行时字符串 `hi` + 换行 · `SEMICOLON` |

**Rust 整数后缀：**

```rust
let n = 0xff_u32;
```

```text
LIT_INT 255 (type: u32)   ← 词法阶段把 0xff 与 _u32 后缀一起读成带类型的字面量 Token
SEMI
```

---

#### 例子 4 · 什么不会变成 Token

| 源码片段 | 扫描器行为 |
|----------|------------|
| `   ` 空格 / `\t` | 跳过 |
| `// comment` | 跳过到行尾 |
| `/* block */` | 跳过（Rust/Lox 规则略不同） |
| `#![allow(...)]` | Rust **属性**，仍由词法/后续阶段处理，不是普通注释 |

Parser **永远看不到**注释文本 — 报错行号仍可能指向原文件第 N 行（Token 上常带 `line` 字段）。

---

#### 例子 5 · 易错边界（自测用）

**1. `=` vs `==`**

```lox
a = 1;   // EQUAL
a == 1;  // EQUAL_EQUAL（多读一个 =）
```

**2. 标识符 vs 关键字**

```lox
var var = 1;   // VAR · IDENTIFIER("var") · EQUAL · NUMBER · SEMICOLON
               // 第二个 var 是变量名，不是关键字
```

**3. 数字紧贴标识符**

```lox
foo123   // 一个 IDENTIFIER "foo123"，不是 foo + 123
123foo   // 通常词法错误：数字后不能直接接字母
```

---

**一句话**：扫描器只做「**切蛋糕**」— 把字符流切成 Token 串；**不管**语法树、不管类型、不管 `=` 两边是否类型匹配（那是 Parser / 语义分析的事）。

**本书对应**：Part II **ch4 Scanning**（jlox）· Part III **ch16 Scanning on Demand**（clox）。

**本仓库**：**03**《自制编译器》词法 · `00-Book` 宏/token 概念 · 深入 [ch4 §4.2 Lexeme 与 Token](../../part02_jlox/chapter04_scanning/02-lexemes-and-tokens.md)

---
