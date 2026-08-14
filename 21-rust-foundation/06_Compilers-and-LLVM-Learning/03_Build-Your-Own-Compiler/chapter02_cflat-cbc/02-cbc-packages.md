# 第 2 章 · C♭ 和 cbc · §2 cbc 的包结构

← [本章目录](./README.md) · 上一节：[01-cflat-language.md](./01-cflat-language.md) · 下一节：[03-compiler-control-flow.md](./03-compiler-control-flow.md)

---

## 实现语言与布局

| 项目 | 说明 |
|------|------|
| **语言** | **Java（Java 5）** |
| **结构** | 标准 Java **目录 = 包** |
| **规模** | 共 **11 个包** |

源码树与包名一一对应 — 读代码时 **按包名导航**。

---

## 两大类包

```text
        cbc 源码
       /        \
  数据相关        处理相关
  (结构/表示)     (Pass/驱动)
```

| 类别 | 职责 | 代表包（书中点名） |
|------|------|-------------------|
| **数据相关** | 程序表示、类型信息 | **`ast`** 抽象语法树 · **`ir`** 中间代码 · **`type`** 类型 |
| **处理相关** | 编译各 Pass | **`compiler`** 核心 · **`parser`** 解析器 |

其余包（共 11 个）支撑 **符号实体、汇编输出、工具** 等 — 随章节逐步进入（如 codegen、entity、asm 等）。

---

## 与 ch1 四阶段的对应

| cbc 内阶段 | 主要包（概念） |
|------------|----------------|
| 语法分析 | **`parser`**（+ JavaCC 生成部分） |
| 语义分析 | **`type`** · **`ast`** 遍历 |
| 中间代码 | **`ir`** |
| 代码生成 | codegen / asm 相关包（第3部分详述） |
| 总控 | **`compiler`** |

→ 不是「一包一阶段」严格隔离，但 **找代码先看表**。

---

## 与 EaC / LLVM 对照

| cbc | 通用名 | LLVM/rustc |
|-----|--------|------------|
| `ast` | AST | Rust HIR-ish / Clang AST |
| `ir` | IR | MIR / LLVM IR |
| `type` | 类型/符号 | TypeCtx / Symbol |
| `parser` | Frontend parse | parser crate |
| `compiler` | Driver | `rustc_driver` |

---

## 自测

- [ ] 数据包 vs 处理包各举两例
- [ ] 改语法规则应主要打开哪个包
- [ ] 11 个包为何比「单文件 main」更易维护
