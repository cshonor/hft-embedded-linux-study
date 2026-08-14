# 第 4 章 · Scanning（扫描 / 词法分析） · §4.3 正则语言与表达式（Regular Languages and Expressions）

← [本章目录](./README.md) · 上一节：[02-lexemes-and-tokens.md](./02-lexemes-and-tokens.md) · 下一节：[04-the-scanner-class.md](./04-the-scanner-class.md)

前置 → [§4.2 Token 定义](./02-lexemes-and-tokens.md) · 流水线 → [ch02 §2.1.1](../../part01_welcome/chapter02_map-of-the-territory/01-1-scanning-lexing.md)

---

## 一、章节定位

| 项 | 说明 |
|----|------|
| **本节** | §4.3 **Regular Languages and Expressions** — 词法规则为何用正则、Scanner 怎么转 |
| **上一节** | [§4.2](./02-lexemes-and-tokens.md) — Lexeme / Token 数据结构 |
| **下一节** | [§4.4](./04-the-scanner-class.md) — **手写** `Scanner` 类落地 |
| **理论延伸** | ch6 **Parser** — 上下文无关文法（CFG），处理嵌套语法 |

**本节核心结论**：Lox 词法规则 = **正则语言**；嵌套结构 = **非正则** → 交给 Parser。

---

## 二、扫描器四步循环（固定流程）

Scanner 本质是处理字符流的 **until EOF** 循环：

```text
while 未到 EOF:
  1. 匹配识别  — 看当前字符，决定哪类词素（数字/标识符/运算符…）
  2. 字符消耗  — 移动指针，吃掉组成该词素的全部连续字符
  3. 产出 Token — 封装 type + lexeme + literal + line
  4. 终止判断  — 未到 EOF → 回到 1；到 EOF → 追加 EOF Token，结束
```

### 伪代码（完整流程）

```text
function scanTokens(source):
    initialize start=0, current=0, line=1
    tokens = []

    while not isAtEnd():
        start = current
        c = peek()                    // 1. 匹配：看当前字符

        if c is whitespace: skip
        elif c is '\n': line++; advance()
        elif c is digit: scanNumber() // 2. 消耗 + 3. addToken
        elif c is alpha: scanIdentifier()
        elif c is '"': scanString()
        elif c is '(' : addToken(LEFT_PAREN); advance()
        elif c is '!': scanBangOrBangEqual()
        else: error(line, "Unexpected character.")

    tokens.add(Token(EOF, "", null, line))
    return tokens

function scanNumber():
    while isDigit(peek()): advance()
    if peek() == '.' and isDigit(peekNext()): advance(); while isDigit(peek()): advance()
    text = source[start..current)
    value = parseDouble(text)
    addToken(NUMBER, value)           // 3. 产出 Token

function addToken(type, literal):
    text = source.substring(start, current)
    tokens.add(new Token(type, text, literal, line))
```

→ 与 [§4.2 伪代码](./02-lexemes-and-tokens.md#七scanner-伪代码切割--生成-token) 衔接 · 实现见 [§4.4～§4.7](./04-the-scanner-class.md)

---

## 三、两种实现路线对比

### 1. 自动生成（工业主流 · Lex / Flex）

| 项 | 说明 |
|----|------|
| **输入** | 每类 Token 一条 **正则表达式** + 动作代码 |
| **工具** | Lex → Flex；Java 生态 **JFlex**；Rust **`logos`** 等 |
| **输出** | 完整 DFA + `yylex()` 风格扫描器 |
| **优点** | 开发快、成熟、规则改起来直观 |
| **缺点** | 匹配原理封装在工具里，不利于「第一遍弄懂每一行」 |

**Flex 规则片段（概念）：**

```text
[0-9]+(\.[0-9]+)?    { return NUMBER; }
[a-zA-Z_][a-zA-Z0-9_]* { return IDENTIFIER; }
"var"                 { return VAR; }
```

**本仓库**：**03 BYOC** 用 **JavaCC** 生成词法+语法 — 对照「生成 vs 手写」取舍。

### 2. 手工手写（本书 jlox / clox）

| 项 | 说明 |
|----|------|
| **做法** | `switch` / `if`、手写 `advance()`、`match()`、边界判断 |
| **目的** | ch1 承诺：**完全掌握**词法每一行在做什么 |
| **优点** | 字符消耗、lookahead、报错行号 **全程可见** |
| **缺点** | 每种词素都要手写；规则多了维护成本高 |

```text
本书路线:  jlox/ch4～7 手写 Java Scanner
           clox/ch16    按需扫描（on demand）
工业路线:  rustc + ...  手写或内部工具链（非教学向 Flex）
```

---

## 四、正则语言的分工边界

### 词法 = 正则语言（Scanner 负责）

**能**用正则描述的：线性、无嵌套、局部看几个字符就够。

| 词素 | 正则思路（示意） |
|------|------------------|
| 标识符 | `[A-Za-z_][A-Za-z0-9_]*` |
| 整数/小数 | `[0-9]+(\.[0-9]+)?` |
| 字符串 | `"` … `"`（转义另计，仍可用正则+状态） |
| 关键字 | 保留字表或正则与标识符分流 |
| `!=` `==` | `!` + `=` 的 **maximal munch** |

### 语法 = 非正则（Parser 负责 · ch6 起）

**不能**用纯正则表达的：需要**嵌套、递归、配对**。

| 结构 | 为何非正则 |
|------|------------|
| 嵌套括号 `( ( ) )` | 需计数/栈 — 超出有限自动机 |
| 表达式 `1+2*3` 的**树形结构** | 正则只认线性串，不认层级 |
| `if { while { } }` 块嵌套 | 同类 — **CFG** |
| 函数/类定义嵌套 | Parser + 文法规则 |

```text
Scanner:  "var" "(" "x" ")" "=" "1" ";"   ← 扁平 Token 串（正则够）
Parser:   Stmt.Var · Call · Block …       ← 树形 AST（要 CFG）
```

→ [ch02 §2.1.2 解析](../../part01_welcome/chapter02_map-of-the-territory/01-2-parsing.md) · [ch6 递归下降](../chapter06_parsing-expressions/README.md)

### 正则的硬限制（直觉）

| 正则**能** | 正则**不能** |
|------------|--------------|
| 匹配固定模式串 | 匹配**任意深度**嵌套 `((…))` |
| 关键字、数字、运算符 | 判断整段程序括号是否平衡 |
| 跳过 `//` 到行尾注释 | 解析完整表达式优先级树 |

> 经典结论：**正则语言 ⊂ 上下文无关语言**；词法是 CFG 的「简单子集」，故拆给 Scanner。

---

## 五、分层设计（Scanner / Parser 分离）

| 好处 | 说明 |
|------|------|
| **职责分离** | Scanner 只管「切 Token」；Parser 只管「拼语法树」 |
| **Scanner 高效** | 跳过空白/注释、维护 `line`、做字面量预处理 |
| **Parser 输入规整** | 只见 `VAR IDENTIFIER EQUAL NUMBER SEMICOLON`，不见 `\t//comment` |
| **工具链复用** | 同一 Flex 词法 + 不同 Bison 语法（多前端实验） |

```text
杂乱字符流  ──Scanner（正则层）──→  Token 流  ──Parser（CFG 层）──→  AST
```

**若合并为一遍**：可行（§2.2.1 单遍编译器），但 Parser 要边读字符边想语法 — 教学与调试都更难；本书 **刻意拆分**。

---

## 六、Lox 词法规则 ↔ 正则 速查

| Token 类 | 手写实现节 | 正则/规则要点 |
|----------|------------|---------------|
| 单字符 `(){};,.*-+%` | §4.5 | 单字符匹配 |
| `!` `!=` `=` `==` … | §4.5 | **lookahead** `match('=')` |
| `//` 注释 | §4.6 | 非嵌套，读到 `\n` |
| 字符串 `"…"` | §4.6 | 转义 `\"` `\n` |
| 数字 | §4.6 | 整数 + 可选 `.` + 小数 |
| 标识符/关键字 | §4.7 | 字母开头 + **保留字表** |

---

## 七、工业与本仓库对照

| 场景 | 方案 |
|------|------|
| **本书 jlox** | 手写 Scanner（本节理论 → §4.4 代码） |
| **03 BYOC** | JavaCC 生成 lexer/parser |
| **rustc** | 手写 `rustc_lexer`（性能 + Rust 特有词法） |
| **快速原型** | Flex / logos / pest 词法部分 |

---

## 八、速记

1. **Scanner 循环**：识别 → 消耗 → `addToken` → 直到 **EOF**。  
2. **词法 = 正则**；**语法嵌套 = CFG → Parser**。  
3. **Flex 自动生成 vs 本书手写** — 弄懂原理选后者。  
4. **分层**：字符流 → Token 流 → AST，各干一层。

---

## 自测

- [ ] 写出 Scanner 四步循环；说明第 2 步「消耗」为何不能省  
- [ ] 举 2 个「正则能处理」和 2 个「必须 Parser」的 Lox 结构  
- [ ] 对比 Flex 与本书手写：各 1 优点 1 缺点  
- [ ] 说明为何 Parser 不应直接扫原始源码里的空格和注释  

→ 下一节：[04 Scanner 类](./04-the-scanner-class.md)
