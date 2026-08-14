# 第 4 章 · Scanning（扫描 / 词法分析） · §4.2 词素与词法单元（Lexemes and Tokens）

← [本章目录](./README.md) · 上一节：[01-the-interpreter-framework.md](./01-the-interpreter-framework.md) · 下一节：[03-regular-languages-and-expressions.md](./03-regular-languages-and-expressions.md)

概念对照 → [ch02 §2.1.1 扫描](../../part01_welcome/chapter02_map-of-the-territory/01-1-scanning-lexing.md) · 脚手架 → [§4.1](./01-the-interpreter-framework.md)

---

## 一、章节定位

| 项 | 说明 |
|----|------|
| **本节** | §4.2 **Lexemes and Tokens** — Scanner **输出什么** |
| **上一节** | [§4.1](./01-the-interpreter-framework.md) — `Lox.java` 框架、`error(line)` |
| **下一节** | [§4.3](./03-regular-languages-and-expressions.md) — 正则语言、词法规则形式化 |
| **再往后** | [§4.4 Scanner 类](./04-the-scanner-class.md) — 用本节字段实现 `scanTokens()` |

**本节不写代码实现**，只定义 Scanner 与 Parser 之间的**数据契约**。

---

## 二、核心概念对照

| 术语 | 核心定义 | 直观例子 |
|------|----------|----------|
| **Lexeme 词素** | 源码里**连续、具备独立语义**的原始字符片段；肉眼可见的**纯文本**，无分类、无附加元数据 | `fun`、`score`、`99`、`=`、`"hello"` |
| **Token 词法单元** | Scanner 对词素**封装**后的结构化对象；带类型、字面量、位置，供 **Parser** 消费 | `VAR` + lexeme `"var"` + line 1 |

### 数据流

```text
源代码字符流
  → 切分字符串 Lexeme（词素）
  → 附加 Type / Literal / Line
  → Token（词法单元）列表
  → Parser（不再直接读源码字符串）
```

---

## 三、Token 标准字段

### 1. `Type`（标记类型 · 枚举）

给词素**归类** — Parser 靠 `token.type` 分支，不靠字符串比较 `== "var"`。

| 类别 | jlox 示例 |
|------|-----------|
| **关键字** | `VAR` `FUN` `IF` `ELSE` `WHILE` `FOR` `CLASS` … |
| **标识符** | `IDENTIFIER`（变量名、函数名、尚未判定的文本） |
| **字面量** | `NUMBER` `STRING` |
| **运算符** | `PLUS` `MINUS` `STAR` `SLASH` `BANG` `BANG_EQUAL` `EQUAL_EQUAL` … |
| **标点** | `LEFT_PAREN` `RIGHT_PAREN` `LEFT_BRACE` `COMMA` `SEMICOLON` … |
| **哨兵** | `EOF`（流结束，Parser 循环条件） |

### 2. `Literal`（运行时值）

把**文本词素**转成程序执行时用的 Java 对象；关键字/运算符通常 **无 literal**。

| Lexeme 文本 | Type | Literal（jlox） |
|-------------|------|-----------------|
| `3.14` | `NUMBER` | `3.14`（`Double`） |
| `"lox"` | `STRING` | `"lox"`（`String`，已去引号、处理转义） |
| `fun` | `FUN` | `null` |
| `+` | `PLUS` | `null` |
| `myVar` | `IDENTIFIER` | `null`（名字在 `lexeme` 字段，见下） |

> jlox 的 `Token` 还保存 **lexeme 字符串**（`var`/`age` 原文），供报错与标识符名使用 — 见书中 `Token.java`。

### 3. `Location`（位置 · 行号为主）

| 作用 | 说明 |
|------|------|
| **报错定位** | `Lox.error(token.line, …)` — 见 [§4.1](./01-the-interpreter-framework.md) |
| **jlox 最小实现** | 仅 **`line`**（`int`） |
| **工业扩展** | 列号、字节偏移、源码 span（`rustc`） |

**例**：第 12 行非法字符 `@` → Scanner 调用 `error(12, "Unexpected character.")`。

---

## 四、完整转换实例

**源码：**

```lox
var age = 18;
```

### 步骤 1 · 拆分 Lexeme（纯文本）

```text
var · age · = · 18 · ;
```

（空白已丢弃，不产生 Lexeme。）

### 步骤 2 · 封装 Token

| # | Lexeme | Type | Literal | Line |
|---|--------|------|---------|------|
| 1 | `var` | `VAR` | — | 1 |
| 2 | `age` | `IDENTIFIER` | — | 1 |
| 3 | `=` | `EQUAL` | — | 1 |
| 4 | `18` | `NUMBER` | `18.0` | 1 |
| 5 | `;` | `SEMICOLON` | — | 1 |
| 6 | — | `EOF` | — | 2 |

**概念输出（线性 Token 流）：**

```text
VAR("var",null,1) · IDENTIFIER("age",null,1) · EQUAL("=",null,1)
· NUMBER("18",18.0,1) · SEMICOLON(";",null,1) · EOF("",null,2)
```

### 步骤 3 · Parser 视角

Parser **只看见 Token 序列**，用 `match(VAR)`、`check(IDENTIFIER)` 等消费 — 不再扫描 `char[]`。

---

## 五、更多转换例子

### 关键字 vs 同名标识符

```lox
var var = 1;
```

| Lexeme | Type | 说明 |
|--------|------|------|
| 第 1 个 `var` | `VAR` | 保留字表命中 |
| 第 2 个 `var` | `IDENTIFIER` | 同名但作变量名（§4.7） |

### 多字符运算符

```lox
a != b
```

| Lexeme | Type |
|--------|------|
| `a` | `IDENTIFIER` |
| `!=` | `BANG_EQUAL`（**一个词素 → 一个 Token**） |
| `b` | `IDENTIFIER` |

### 字符串字面量

```lox
print "hi\n";
```

| Lexeme 原文 | Type | Literal |
|-------------|------|---------|
| `"hi\n"` | `STRING` | 两字符字符串 `hi` + 换行 |

---

## 六、Lexeme vs Token 三条区分

1. **Lexeme 只是字符串** — 无类型、无数值，是人眼在源码里看到的那一段。  
2. **Token 是结构化数据** — Parser 的**唯一**词法输入；不回头摸原始 `source`。  
3. **一对多（Type 侧）** — 同一 `Type`（如 `IDENTIFIER`）可对应不同 Lexeme（`a`、`score`、`orchid`）；**一个 Lexeme 扫描结果对应一个 Token**。

```text
Lexeme "score"  →  Token(IDENTIFIER, lexeme="score", …)
Lexeme "a"      →  Token(IDENTIFIER, lexeme="a", …)
                同属 IDENTIFIER，lexeme 不同
```

---

## 七、Scanner 伪代码（切割 → 生成 Token）

```text
scanTokens(source):
    tokens = []
    start = 0, current = 0, line = 1

    while not atEnd():
        start = current
        char c = advance()

        switch c:
            case ' ', '\r', '\t': break          // 丢弃空白
            case '\n': line++; break
            case '(': addToken(LEFT_PAREN); ...
            case '+': addToken(PLUS); ...
            case '!':
                if match('='): addToken(BANG_EQUAL)
                else:          addToken(BANG)
            case '"': string(); ...              // §4.6
            default:
                if isDigit(c): number(); ...
                elif isAlpha(c): identifier(); ... // §4.7 查关键字表
                else: error(line, "Unexpected character.")

        // addToken 内部:
        //   text = source[start..current)
        //   tokens.add(new Token(type, text, literal, line))

    tokens.add(new Token(EOF, "", null, line))
    return tokens
```

**要点**：`start`/`current` 界定当前 **Lexeme 文本**；`addToken` 把它包装成 **Token**。

→ 完整实现：[§4.4 Scanner 类](./04-the-scanner-class.md) · [§4.5 识别词素](./05-recognizing-lexemes.md)

---

## 八、jlox `Token` 类（书中形状）

```java
class Token {
  final TokenType type;
  final String lexeme;    // 词素原文
  final Object literal;   // NUMBER/STRING 等有值，其余 null
  final int line;

  Token(TokenType type, String lexeme, Object literal, int line) { ... }

  public String toString() {
    return type + " " + lexeme + " " + literal;
  }
}
```

`toString()` 便于 §4.4 调试时打印整条 Token 流。

---

## 九、工业对照

| jlox Token | `rustc` / 通用编译器 |
|------------|----------------------|
| `type` + `lexeme` | `TokenKind` + `symbol` |
| `line` | `Span { lo, hi }` → 行列 |
| `literal` | 字面量在 parser 前已解析为 typed literal |
| `EOF` | `Eof` 哨兵 |

---

## 十、速记

1. **Lexeme = 文本片段；Token = 类型 + lexeme + literal + line。**  
2. **Parser 只吃 Token，不吃源码。**  
3. **同 Type 多 Lexeme**（所有变量都是 `IDENTIFIER`）。  
4. **`EOF` 必追加**，Parser 用 `while (!isAtEnd())` 循环。

---

## 自测

- [ ] 对 `var age = 18;` 手写 6 个 Token（含 `EOF`）的 type / literal  
- [ ] `!=` 是几个 Lexeme、几个 Token？  
- [ ] `NUMBER` 的 literal 在 jlox 里是什么 Java 类型？  
- [ ] 说明 Lexeme 与 `Token.lexeme` 字段的关系  

→ 下一节：[03 正则语言与表达式](./03-regular-languages-and-expressions.md)
