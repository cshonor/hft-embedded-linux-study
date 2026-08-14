# 第 2 章 · A Map of the Territory（领域地图） · §2.2 捷径与替代方案（Shortcuts and Alternate Routes）

← [本章目录](./README.md) · 上一节：[01-the-parts-of-a-language.md](./01-the-parts-of-a-language.md) · 下一节：[03-a-map-of-the-territory.md](./03-a-map-of-the-territory.md)

---

并非每种语言都要走完 §2.1 的**完整长线**；常见「抄近路」：

### §2.2.1 单遍编译器（Single-pass compilers）

| 项目 | 说明 |
|------|------|
| **做法** | 解析、语义分析、代码生成**交织**；解析到一个语法块**立刻**输出目标代码 |
| **特点** | **不在内存中保留完整 AST** → 极省内存 |
| **历史** | 早期 **C**、**Pascal** 等在资源受限环境下的设计 |

**对比本书**：CI 为教学**刻意走多遍、保留 AST/字节码**，便于理解每一站。

---

### §2.2.2 树遍历解释器（Tree-walk interpreters）

| 项目 | 说明 |
|------|------|
| **做法** | 解析成 **AST** 后**直接执行**；在树上逐节点、逐分支求值 |
| **特点** | 实现相对简单；通常**较慢** |
| **本书** | Part II **`jlox`** 即此路线（ch7 起求值遍历 AST） |

```text
Parse → AST → interpret(node) 递归/Visitor
```

**本仓库衔接**：理解 jlox 后再读 **03** 青木书「真编译器」全链路，对比「解释 vs 编译到二进制」。

---

### §2.2.3 转译器（Transpilers）

| 项目 | 说明 |
|------|------|
| **做法** | 不降到机器码/字节码；把源码**翻译**成另一种**高级语言** |
| **例子** | 某语言 → **C** / **JavaScript** 等，再交给现有工具链 |
| **特点** | 借目标语言生态；源码级可读性仍在 |

**对比**：**rustc** 不是 transpiler（降到 LLVM IR / 机器码）；**TypeScript → JS** 是典型的 transpile。

---
