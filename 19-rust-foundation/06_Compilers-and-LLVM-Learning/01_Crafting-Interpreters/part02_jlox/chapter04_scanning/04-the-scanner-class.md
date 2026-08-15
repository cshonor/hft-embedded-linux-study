# 第 4 章 · Scanning（扫描 / 词法分析） · §4.4 Scanner 类（The Scanner Class）

← [本章目录](./README.md) · 上一节：[03-regular-languages-and-expressions.md](./03-regular-languages-and-expressions.md) · 下一节：[05-recognizing-lexemes.md](./05-recognizing-lexemes.md)

前置 → [§4.2 Token](./02-lexemes-and-tokens.md) · [§4.3 扫描循环](./03-regular-languages-and-expressions.md)

---

## 一、章节定位

| 项 | 说明 |
|----|------|
| **本节** | §4.4 **The Scanner Class** — 数据结构 + 三个指针 + `scanTokens()` 骨架 |
| **上一节** | [§4.3](./03-regular-languages-and-expressions.md) — 四步循环、正则 vs CFG |
| **下一节** | [§4.5](./05-recognizing-lexemes.md) — 各类词素识别（`switch`、运算符） |

**本节落地「容器」**；词素具体怎么认在 §4.5～§4.7 填进 `scanToken()`。

---

## 二、输入与输出

| | 说明 |
|---|------|
| **输入** | 完整源码字符串 `String source` |
| **输出** | `List<Token>` — 含末尾 **`EOF`** 哨兵 |
| **jlox** | **一次性**扫完全部源码，返回完整列表 |
| **clox** | Part III 改为 **按需** 返回单个 Token（[ch16 按需扫描](../../part03_clox/chapter16_scanning-on-demand/README.md)）— 省内存、与递归下降编译器同频 |

```java
// §4.4 接入 §4.1 的 run()
private static void run(String source) {
    Scanner scanner = new Scanner(source);
    List<Token> tokens = scanner.scanTokens();
    // 调试: System.out.println(tokens);
}
```

---

## 三、三个核心整型偏移（状态机）

Scanner 不修改 `source`，只用下标在字符串上滑动。

| 变量 | 语义 | 典型时机 |
|------|------|----------|
| **`start`** | 当前词素 **Lexeme 首字符**下标 | 每个新词素开始前 `start = current` |
| **`current`** | **下一个待读**字符下标（扫描指针） | `advance()` 读后 `++` |
| **`line`** | 当前行号（从 **1** 起） | 读到 `\n` 时 `line++` |

### 指针示意图

```text
source:  v  a  r     a  g  e  ...
index:   0  1  2  3  4  5  6
         ^start
         ^current        （词素 "var" 扫描中）

截取: source.substring(start, current)  →  "var"
```

| 操作后 | start | current | 说明 |
|--------|-------|---------|------|
| 读完 `var` | 0→3 | 3 | 词素 `[0,3)` = `"var"` |
| 跳过空格 | 3 | 4 | 空白不产生 Token |
| 开始 `age` | 4 | 4 | `start = current` |

---

## 四、扫描一轮词素的标准流程

```text
1. start = current              // 新词素起点（scanToken 入口常做）
2. while 仍属同一词素:
       advance()                // 消耗字符，current++
3. addToken(type [, literal])   // 用 [start, current) 截取 lexeme
4. （隐式）下一词素时再次 start = current
```

### 走读示例 · 词素 `var`

源码 `"var age"`，从 index 0 开始：

| 步 | 动作 | start | current | 说明 |
|----|------|-------|---------|------|
| 0 | 进入 `identifier()` | 0 | 0 | |
| 1 | `advance()` | 0 | 1 | 读 `v` |
| 2 | `advance()` | 0 | 2 | 读 `a` |
| 3 | `advance()` | 0 | 3 | 读 `r` |
| 4 | `peek()` 是空格，停止 | 0 | 3 | lexeme=`"var"` |
| 5 | `addToken(VAR)` | — | 3 | 查保留字表 → `VAR` |
| 6 | 下一词素 | 3→4 | 4 | 跳过空格后 `start=current` |

> 单字符词素（如 `;`）：`advance()` 一次 → 立即 `addToken(SEMICOLON)`，`start` 与 `current` 仅差 1。

---

## 五、核心工具函数

### 1. `advance()`

```java
private char advance() {
    return source.charAt(current++);
}
```

取出 `source[current]`，然后 **`current++`** — **唯一**「正式消耗」字符的入口（除跳过空白时的 `current++`）。

### 2. `peek()`

```java
private char peek() {
    if (isAtEnd()) return '\0';
    return source.charAt(current);
}
```

**只看不动** — 用于 `=` vs `==`、`!` vs `!=`（§4.5 `match()`）。

### 3. `peekNext()`（§4.6 数字小数点用）

```java
private char peekNext() {
    if (current + 1 >= source.length()) return '\0';
    return source.charAt(current + 1);
}
```

### 4. `isAtEnd()`

```java
private boolean isAtEnd() {
    return current >= source.length();
}
```

`scanTokens()` 主循环条件；结束时追加 `EOF` Token。

### 5. `match(char expected)`（§4.5）

```java
private boolean match(char expected) {
    if (isAtEnd()) return false;
    if (source.charAt(current) != expected) return false;
    current++;
    return true;
}
```

**条件消耗** — 匹配则 `current++`，否则指针不动。

### 6. `addToken(type)` / `addToken(type, literal)`

```java
private void addToken(TokenType type) {
    addToken(type, null);
}

private void addToken(TokenType type, Object literal) {
    String text = source.substring(start, current);
    tokens.add(new Token(type, text, literal, line));
}
```

| 字段来源 | |
|----------|---|
| `lexeme` | `source.substring(start, current)` |
| `line` | 成员变量 `line` |
| `literal` | `number()` / `string()` 里解析好的值 |

---

## 六、`Scanner` 类骨架（本节汇总）

```java
class Scanner {
    private final String source;
    private final List<Token> tokens = new ArrayList<>();
    private int start = 0;
    private int current = 0;
    private int line = 1;

    Scanner(String source) {
        this.source = source;
    }

    List<Token> scanTokens() {
        while (!isAtEnd()) {
            start = current;
            scanToken();           // §4.5 起：switch 分发
        }
        tokens.add(new Token(EOF, "", null, line));
        return tokens;
    }

    private void scanToken() {
        char c = advance();
        switch (c) {
            // §4.5: ( ) { } , . - + ; * ...
            // §4.6: // comment, string, number
            // §4.7: identifier / keyword
            default:
                Lox.error(line, "Unexpected character.");
                break;
        }
    }

    // advance, peek, peekNext, isAtEnd, match, addToken ...
}
```

**设计要点**：

- `scanTokens()` **外层循环** — 一词素一次 `scanToken()`  
- 每个 `scanToken()` 开头 **`start = current`**（书中在 `while` 里设 `start`）  
- 词法错误调用 `Lox.error(line, …)`，通常 **继续扫**（§4.5），不崩进程  

---

## 七、jlox vs clox 扫描架构

| | **jlox §4.4** | **clox ch16** |
|---|---------------|---------------|
| API | `scanTokens()` → `List<Token>` | `scanToken()` → 单个 `Token` |
| 内存 | 整文件 Token 列表 | 不缓存全表 |
| 消费者 | 调试打印 / Parser 一次拿 List | 编译器边 parse 边要下一个 Token |
| 三指针 | `start` / `current` / `line` | **相同思想** |

---

## 八、与前后小节串联

```text
§4.2  Token 长什么样
§4.3  为什么要循环、正则分工
§4.4  Scanner 类 + 三指针 + 工具函数   ← 本节
§4.5  单/双字符运算符 switch
§4.6  注释、字符串、数字
§4.7  标识符、关键字表
```

---

## 九、易错边界

| 易混 | 纠正 |
|------|------|
| `peek()` 也消耗字符 | **不移动** `current`；只有 `advance()`/`match()` 消耗 |
| 忘记 `start = current` | 会把多个词素粘成一个 lexeme |
| `substring(start, current)` | Java **左闭右开** `[start, current)` |
| 换行只增 `line` 不增 Token | `\n` 通常被当空白跳过 |
| EOF 不手动加 | `scanTokens()` 末尾必须 `addToken(EOF)` |

---

## 十、速记

1. **输入 `source` → 输出 `List<Token>` + `EOF`。**  
2. **`start` 定词素头，`current` 定下一待读，`line` 定报错行。**  
3. **一轮词素：对齐 start → advance 吃到头 → addToken。**  
4. **`peek` 预判，`advance` 消耗，`substring(start,current)` 取 lexeme。**

---

## 自测

- [ ] 对 `"var"` 画出 start/current 每步变化（见 §四表）  
- [ ] 说明 `peek()` 与 `advance()` 在识别 `!=` 时的分工  
- [ ] jlox 一次性列表 vs clox 按需 Token 各适合什么消费者？  
- [ ] `addToken` 里 lexeme 从哪两个变量截取？  

→ 下一节：[05 识别词素](./05-recognizing-lexemes.md)
