# 第 2 章 · C♭ 和 cbc · §3 编译控制与流程

← [本章目录](./README.md) · 上一节：[02-cbc-packages.md](./02-cbc-packages.md) · 下一节：---

## 核心：`compiler` 包 · `Compiler` 类

**cbc 的大脑** — 统管从源文件到可执行文件的 **整体处理**。

---

## `build`：多文件构建

```text
for each 源文件:
    compile(源文件)     →  汇编（+ 中间产物）
assemble(...)         →  目标文件 .o
link(..., 库)         →  可执行 ELF
```

| 步骤 | 函数 | 对应 [ch1 Build 链](../chapter01_start/01-book-overview.md) |
|------|------|-------------------------------------------------------------|
| 逐文件编译 | **`compile`** | 狭义 **编译**（→ 汇编） |
| 汇编 | **`assemble`** | **汇编** |
| 链接 | **`link`** | **链接** |

**循环**：多个 `.cb` 各走一遍 `compile`，再 **一起** assemble/link — 与 `gcc *.c` 同构。

---

## `compile`：单文件四步

**一章内最重要的微观流水线** — 与 [ch1 §2 四阶段](../chapter01_start/02-four-compiler-stages.md) 一一对应：

```text
  .cb 源文件
     ↓
  ① parse        →  AST（抽象语法树）
     ↓
  ② 语义分析      →  类型/作用域等（仍基于 AST）
     ↓
  ③ 生成 IR      →  中间代码
     ↓
  ④ 代码生成      →  x86 汇编写入文件
```

| 步骤 | 产出 | 后续章节 |
|------|------|----------|
| 解析 | **AST** | 第1部分 parser · 第2部分 ch7～8 |
| 语义分析 | 校验后的 AST | ch9～10 |
| IR | cbc IR | ch11 |
| 代码生成 | `.s` 等 | 第3部分 ch12～17 |

---

## 全景：两层函数

```text
Compiler.build()
  ├─ compile(file₁)  : parse → sema → IR → asm
  ├─ compile(file₂)  : …
  ├─ assemble(all)
  └─ link(all + libs)  →  a.out / hello
```

**阅读源码建议**：先 **`Compiler.build`/`compile` 骨架**，再按四步 **钻进各包** — 与本书章节顺序一致。

---

## 与 Rust / gcc

| | cbc | gcc | rustc |
|---|-----|-----|-------|
| 驱动类 | `Compiler` | driver main | `rustc_driver` |
| 单文件 | `compile` | cc1 | 前端+MIR+LLVM emit |
| 外部工具 | assemble/link 调 GNU | as/ld | linker |

---

## 自测

- [ ] `build` 三步 vs `compile` 四步各做什么
- [ ] 多源文件时 `compile` 与 `link` 如何配合
- [ ] `import stdio` 在 link 阶段可能依赖什么
