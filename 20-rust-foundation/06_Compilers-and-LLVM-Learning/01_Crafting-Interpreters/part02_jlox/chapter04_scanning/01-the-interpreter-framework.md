# 第 4 章 · Scanning（扫描 / 词法分析） · §4.1 解释器框架（The Interpreter Framework）

← [本章目录](./README.md) · 上一节：[00-overview.md](./00-overview.md) · 下一节：[02-lexemes-and-tokens.md](./02-lexemes-and-tokens.md)

流水线第一站 → [ch02 §2.1.1 扫描](../../part01_welcome/chapter02_map-of-the-territory/01-1-scanning-lexing.md)

---

## 一、章节定位与上下文

| 项 | 说明 |
|----|------|
| **本章** | **Scanning**（扫描 / 词法分析 Lexing） |
| **本节** | §4.1 **The Interpreter Framework** — jlox Java 顶层脚手架 |
| **上一节** | [00-overview](./00-overview.md) — Part II 起点、流水线第一站 |
| **下一节** | [02-lexemes-and-tokens](./02-lexemes-and-tokens.md) — 词素、Token 数据结构 |

**核心前置逻辑**：实现 **Scanner** 之前，必须先搭好解释器顶层调度、双运行模式、统一错误上报 — **后续 Scanner / Parser 全程复用** `run()` 与 `error()`。

```text
本节:  Lox.java 脚手架（main / runFile / REPL / error）
下节:  Lexeme vs Token 定义
§4.4+: Scanner 类实现，输出 Token 流
```

---

## 二、jlox 两种运行入口

Lox 是脚本语言，支持 **批处理文件** 与 **交互 REPL**，由 `Lox.java` 主类调度。

### 1. `runFile(String path)` — 文件模式

| 步骤 | 动作 |
|------|------|
| 1 | 读取目标文件全部字节 |
| 2 | 按默认字符集转 **UTF-8 源码字符串** |
| 3 | 调用统一 `run(source)`（本节为空壳；§4.4 起接 Scanner） |
| 4 | 若 `hadError` → **`System.exit(65)`**（UNIX sysexits：数据格式错误） |

```java
private static void runFile(String path) throws IOException {
    byte[] bytes = Files.readAllBytes(Paths.get(path));
    run(new String(bytes, Charset.defaultCharset()));
    if (hadError) System.exit(65);
}
```

### 2. `runPrompt()` — REPL 交互模式

| 步骤 | 动作 |
|------|------|
| 1 | 循环读控制台一行 |
| 2 | 每行调用 `run(line)` |
| 3 | **单行报错不终止** — 执行后 `hadError = false` |
| 4 | `readLine() == null`（Ctrl+D / EOF）→ 退出 |

```java
private static void runPrompt() throws IOException {
    InputStreamReader input = new InputStreamReader(System.in);
    BufferedReader reader = new BufferedReader(input);
    for (;;) {
        System.out.print("> ");
        String line = reader.readLine();
        if (line == null) break;
        run(line);
        hadError = false;  // 单行错误不影响下一行
    }
}
```

### 3. `main` 命令行分支

| `args.length` | 行为 | 退出码 |
|---------------|------|--------|
| **> 1** | 打印 `Usage: jlox [script]` | **64**（命令行用法错误） |
| **= 1** | `runFile(args[0])` | 有错则 65 |
| **0** | `runPrompt()` REPL | 正常 EOF 退出 |

```java
public static void main(String[] args) throws IOException {
    if (args.length > 1) {
        System.out.println("Usage: jlox [script]");
        System.exit(64);
    } else if (args.length == 1) {
        runFile(args[0]);
    } else {
        runPrompt();
    }
}
```

### 4. 统一 `run(String source)` — 占位

本节 **Scanner 尚未实现**，仅预留接口：

```java
private static void run(String source) {
    // §4.4 起: Scanner scanner = new Scanner(source);
    //         List<Token> tokens = scanner.scanTokens();
}
```

---

## 三、统一错误处理体系（本节重点）

### 1. 全局标记

```java
static boolean hadError = false;
```

| 模式 | `hadError` 行为 |
|------|-----------------|
| **文件** | 任意错误 → 保持 `true` → `runFile` 末尾 `exit(65)` |
| **REPL** | 每行 `run` 后 **重置为 `false`** |

### 2. 对外接口 `error(int line, String message)`

Scanner、Parser 统一调用 — 必须带 **行号**：

```java
static void error(int line, String message) {
    report(line, "", message);
}
```

### 3. 底层 `report()`

输出到 **`System.err`**，并置 `hadError = true`：

```java
private static void report(int line, String where, String message) {
    System.err.println("[line " + line + "] Error" + where + ": " + message);
    hadError = true;
}
```

**输出示例：**

```text
[line 5] Error: unexpected character '@'
```

`where` 预留给 Parser 扩展（如 `" at 'foo'"`）— §6.3 语法错误会用到。

### 4. 四条设计原则

1. **精准行号** — 词法/语法错误必须绑定源码行；Scanner 维护 `line` 字段（§4.4）。
2. **错误隔离** — 非法字符、语法错误 → 只上报，**不执行**残缺 Token/AST。
3. **双模式容错** — 文件：有错即停；REPL：单行失败继续输入。
4. **语言设计一环** — 错误处理从 **词法阶段** 就要完整，不是事后补丁（呼应 ch1 Design Notes）。

---

## 四、文件模式 vs REPL 对照

| 维度 | `runFile` | `runPrompt` |
|------|-----------|-------------|
| 输入 | 整个 `.lox` 文件 | 控制台一行 |
| 出错后 | **退出 65** | **继续** `>` 提示 |
| `hadError` | 累积到文件结束 | **每行清零** |
| 典型用途 | 脚本批跑、测试 | 探索语法、调试 |

---

## 五、工业级对照（延伸）

| jlox（本节） | `rustc` / 工业编译器 |
|--------------|----------------------|
| `[line N] Error: msg` | 行号 + **列号** + 源码片段高亮 |
| 单全局 `hadError` | 多错误聚合、错误码 `E0xxx` |
| `System.err` 文本 | 结构化 JSON / `rustc --error-format` |
| 有错不执行 | 同上 + 尽量多报再停 |

**要点**：jlox 是诊断体系的 **最小可工作原型**；写 Scanner 时第一优先级仍是 **行号追踪准确**（换行、`//` 注释、字符串内 `\n` 等 §4.6 再细抠）。

---

## 六、后续章节衔接

| 章节 | 与本节关系 |
|------|------------|
| [02 Lexeme & Token](./02-lexemes-and-tokens.md) | Scanner **输出数据结构** |
| [04 Scanner 类](./04-the-scanner-class.md) | `run()` 内 `new Scanner(source).scanTokens()` |
| [05～07 识别词素](./05-recognizing-lexemes.md) | 全程调用 `Lox.error(line, …)` |
| ch6 Parser | 复用 `error()` + `hadError`；文件模式仍 `exit(65)` |

---

## 七、本节完整 `Lox.java` 脚手架

```java
package com.craftinginterpreters.lox;

import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStreamReader;
import java.nio.charset.Charset;
import java.nio.file.Files;
import java.nio.file.Paths;

public class Lox {
    static boolean hadError = false;

    public static void main(String[] args) throws IOException {
        if (args.length > 1) {
            System.out.println("Usage: jlox [script]");
            System.exit(64);
        } else if (args.length == 1) {
            runFile(args[0]);
        } else {
            runPrompt();
        }
    }

    private static void runFile(String path) throws IOException {
        byte[] bytes = Files.readAllBytes(Paths.get(path));
        run(new String(bytes, Charset.defaultCharset()));
        if (hadError) System.exit(65);
    }

    private static void runPrompt() throws IOException {
        InputStreamReader input = new InputStreamReader(System.in);
        BufferedReader reader = new BufferedReader(input);
        for (;;) {
            System.out.print("> ");
            String line = reader.readLine();
            if (line == null) break;
            run(line);
            hadError = false;
        }
    }

    private static void run(String source) {
        // §4.4 起实现 Scanner
    }

    static void error(int line, String message) {
        report(line, "", message);
    }

    private static void report(int line, String where, String message) {
        System.err.println("[line " + line + "] Error" + where + ": " + message);
        hadError = true;
    }
}
```

---

## 八、速记

1. **`main`**：`0` 参数 REPL · `1` 参数跑文件 · `>1` 用法错误 **64**。  
2. **文件有错 `exit(65)`**；REPL **每行重置 `hadError`**。  
3. **`error(line, msg)`** — Scanner/Parser 统一入口；**有错不跑残缺代码**。  
4. **`run(source)`** — 全流水线总线；本节空壳，§4.4 接 Scanner。

---

## 自测

- [ ] 说明退出码 **64** 与 **65** 分别对应什么情况  
- [ ] REPL 里一行非法字符后，为何还能继续输入下一行？  
- [ ] `report` 的 `where` 参数预留给谁用？  
- [ ] 对照 [ch02 §2.1.1](../../part01_welcome/chapter02_map-of-the-territory/01-1-scanning-lexing.md)：Scanner 产出什么、`run()` 下一步接谁？

→ 下一节：[02 词素与 Token](./02-lexemes-and-tokens.md)
