# 第 4 章 · 上下文相关分析 · §1 为何需要

← [本章目录](./README.md) · 上一节：[00-overview.md](./00-overview.md) · 下一节：[02-type-systems.md](./02-type-systems.md)

---

## CFG / 语法分析的局限

扫描器 + 语法分析器只处理**局部、上下文无关**的规则：

| 能检查 | 不能检查（需 ch4） |
|--------|---------------------|
| 括号是否配对 | **变量先声明后使用** |
| 语句结构是否正确 | **`x + y` 类型是否兼容** |
| 表达式语法合法 | **数组维数是否匹配** |
| `if`/`while` 形态 | **函数实参与形参是否一致** |

> 语法正确 ≠ 程序合法。

---

## 上下文相关分析做什么

查明语法的**实际意义**，发现**深层次错误** — 即 **语义分析**。

| 任务示例 | 说明 |
|----------|------|
| **符号解析** | 名字绑定到哪个声明 |
| **作用域** | 内层是否遮蔽外层 |
| **类型检查** | 运算、赋值、调用是否类型安全 |
| **约束检查** | 语言特定的上下文规则 |

---

## 与 CI / Rust 对照

| 体系 | ch4 对应物 |
|------|------------|
| **jlox ch11** | **Resolver** — 静态解析变量、解决闭包槽位 |
| **jlox ch7** | 运行时类型检查（Lox 动态类型） |
| **Rust** | **`rustc` 语义阶段** — 类型 + 借用 + 生命周期（比 Lox 严格得多） |

→ [CI ch11 Resolving](../../../01_Crafting-Interpreters/part02_jlox/chapter11_resolving-and-binding/README.md)

---

## 在流水线中的位置

[ch1 理解输入 · 语义](../chapter01_overview/04-translation-pipeline-example.md) · [CI 上山前端 · 语义分析](../../../01_Crafting-Interpreters/part01_welcome/chapter02_map-of-the-territory/04-rust-hft-编译流水线对照.md)

**输出**：带语义信息的 **AST**、**符号表**、类型标注 — 供 ch5 **IR**  lowering。
