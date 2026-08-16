# 第 8 章 · 抽象语法树的生成 · §4 cbc 解析器的启动

← [本章目录](./README.md) · 上一节：[03-decl-ast.md](./03-decl-ast.md) · 下一节：---

## Parser 对象初始化

**`Parser` 专用构造函数** 准备解析上下文：

| 组件 | 作用 |
|------|------|
| **源文件名** | 错误信息、位置 |
| **`LibraryLoader`** | **`import`** 时加载 `.hb` 库 |
| **错误处理器** | 统一报告词法/语法错误 |
| **`typedef` 表** | 解析过程中记录 typedef 名 |
| **调试选项** | 可选 **跟踪日志**（JavaCC trace） |

```text
new Parser(input, fileName, loader, …)
```

**与 JavaCC 生成类**：手写 **`Parser` 子类/包装** 或在 `PARSER_BEGIN` 块扩展成员 — cbc 在 **`parser` 包**。

---

## 启动解析

| API | 说明 |
|-----|------|
| **`parseFile(path)`** | **静态** — 打开文件流，构造 Parser，解析 |
| **`parse()`** | **实例** — 对已有输入流执行解析 |

**核心调用**：

```text
parse() {
  try {
    AST ast = compilation_unit();   // 根规则 — ch6
    return ast;
  } catch (ParseException | TokenMgrError …) {
    // 转换为用户可见错误
  }
}
```

| 异常来源 | 含义 |
|----------|------|
| **`TokenMgrError`** | **词法**错误（ch4） |
| **`ParseException`** | **语法**错误（ch5～6） |

---

## 与 compile 管线

```text
Compiler.compile(file)
  → Parser.parseFile(file)  →  AST     ← 本章终点
  → SemanticAnalyzer…       →  ch9～10
  → IRGenerator…            →  ch11
```

**调试**：

```bash
cbc --dump-ast hello.cb    # Node.dump() — ch7
# + Parser 调试 trace
```

---

## 自测

- [ ] Parser 构造需要哪四类上下文
- [ ] `parse()` 调用的根非终端名
- [ ] 词法异常 vs 语法异常各来自哪
