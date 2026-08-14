# 第 5 章 · 中间表示（Intermediate Representation, IR）

> **Engineering a Compiler 3e** · [02 编译器工程](../../README.md) · [本书目录](../../本书目录.md) · Part II 基础结构

## 状态

- [x] 已读（笔记整理）

---

## 一句话

**IR = 前端与后端的桥梁** — 独立于源语言与目标机，表达「正在被翻译的程序」。本章讲 **IR 为何需要、如何分类（图/线/混合）**、**AST/DAG/CFG**、**栈机/三地址/ILOC**、**SSA**、**寄存器 vs 内存模型** 与 **符号表** — 决定后续能做多深的优化。

---

## 专项笔记（一小节一文件）

| 小节 | 主题 | 阅读 |
|------|------|------|
| — | 本章定位 | [00-overview.md](./00-overview.md) |
| §1 | 为何使用 IR | [01-why-ir.md](./01-why-ir.md) |
| §2 | IR 分类（图 / 线 / 混合） | [02-ir-taxonomy.md](./02-ir-taxonomy.md) |
| §3 | 图示 IR（AST · DAG · CFG） | [03-graphical-ir.md](./03-graphical-ir.md) |
| §4 | 线性 IR（栈机 · 三地址 · ILOC） | [04-linear-ir.md](./04-linear-ir.md) |
| §5 | SSA 静态单赋值 | [05-ssa-form.md](./05-ssa-form.md) |
| §6 | 名字映射与内存模型 | [06-names-and-memory-models.md](./06-names-and-memory-models.md) |
| §7 | 符号表 | [07-symbol-tables.md](./07-symbol-tables.md) |
| — | 速记 · 自测 |

---

## 与仓库其他部分

| 本书 ch5 | 对照 |
|----------|------|
| AST | [ch4 SDT](../chapter04_context/04-syntax-directed-translation.md) · [CI jlox ch5](../../../01_Crafting-Interpreters/part02_jlox/chapter05_representing-code/README.md) |
| 栈机 / 字节码 | [CI clox Chunk](../../../01_Crafting-Interpreters/part03_clox/chapter14_chunks-of-bytecode/README.md) |
| SSA / LLVM IR | [04 Learn LLVM 17](../../../04_Learn-LLVM-17/README.md) · ch8～10 优化 |
| 符号表 | [ch4](../chapter04_context/README.md) · jlox ch11 Resolver |

---

## 逻辑脉络

为何 IR → 形态分类 → 具体 IR 族 → SSA → 命名/存储假设 → 符号表。

---

## 速记

## 本章速记

```text
§1  IR=前后端桥梁 · m×n · 优化基础
§2  图IR / 线IR / 混合(CFG块+块内线)
§3  AST(嵌套) · DAG(共享子expr) · CFG(基本块+边)
§4  栈机(紧凑/clox/JVM) · 三地址/ILOC(分析/regalloc)
§5  SSA: 每名一次赋值 · φ在合流点 · 简化数据流
§6  reg-to-reg(现代) vs mem-to-mem(慢) · 虚拟寄存器
§7  符号表: 类型/作用域/存储 · 散列+嵌套scope
```

---

## 三句背诵

1. **IR 解耦源与目标，是优化的主战场。**
2. **AST 看结构，CFG 看控制，三地址/SSA 看数据流。**
3. **SSA + φ 让 def-use 清晰；符号表管源名，IR 名管临时值。**

---

## 与 CI / LLVM 对照

| 橡书 ch5 | 本仓库 |
|----------|--------|
| AST | jlox |
| 栈线性 IR | clox 字节码 |
| SSA | LLVM IR · ch8～10 |
| 符号表 | jlox ch11 |

---

## 自测

- [ ] m×n 架构什么意思
- [ ] AST vs DAG vs CFG 各表达什么
- [ ] 栈机 vs 三地址 各适合什么
- [ ] SSA 规则 + φ 何时出现
- [ ] reg-to-reg 为何是现代默认
- [ ] 符号表如何处理作用域遮蔽

---

## 阅读进度

- [x] ch5 中间表示（本章笔记）
- [ ] ch6 过程抽象

