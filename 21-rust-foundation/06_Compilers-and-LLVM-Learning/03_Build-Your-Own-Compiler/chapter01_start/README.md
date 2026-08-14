# 第 1 章 · 开始制作编译器

> **《自制编译器》** · [03 Build Your Own Compiler](../../README.md) · [本书目录](../../本书目录.md) · 开篇

## 状态

- [x] 已读（笔记整理）

---

## 一句话

**全书地图章** — 目标：用 **Java/JavaCC** 实现 **cbc**，把 **C♭** 编译为 **x86 Linux ELF**；厘清 **Build 四步**（预处理→编译→汇编→链接）与 **cbc 内四步**（语法→语义→IR→代码生成）+ 优化； **`cbc hello.cb` → `./hello`** 建立可执行文件直觉。

---

## 专项笔记（一小节一文件）

| 小节 | 主题 | 阅读 |
|------|------|------|
| — | 本章定位 | [00-overview.md](./00-overview.md) |
| §1 | 本书的概要 | [01-book-overview.md](./01-book-overview.md) |
| §2 | 编译的 4 个主要阶段 | [02-four-compiler-stages.md](./02-four-compiler-stages.md) |
| §3 | 使用 C♭ 编译器进行编译 | [03-cbc-demo.md](./03-cbc-demo.md) |
| — | 速记 · 自测 |

---

## 与仓库其他部分

| 本书 ch1 | 对照 |
|----------|------|
| 三阶段 / 流水线 | [CI ch2 编译之山](../../../01_Crafting-Interpreters/part01_welcome/chapter02_map-of-the-territory/04-rust-hft-编译流水线对照.md) |
| 编译总览 | [EaC ch1](../../../02_Compiler-Principles/chapter01_overview/README.md) |
| ch2 下一章 | [chapter02_cflat-cbc/](../chapter02_cflat-cbc/README.md) · C♭ 语法与 cbc 结构 |
| Rust 编译链 | RFR [05 编译与分发](../../../../02-RFR/Chapter-02-Types/05-compilation-dispatch.md) |

---

## 逻辑脉络

概要与环境 → cbc 内部分阶段 → 动手跑通 Hello World。

---

## 速记

## 本章速记

```text
§1  C♭ 子集+C指针 · x86 Linux ELF · Build四步：预处理/编译/汇编/链接
§2  cbc内四步：语法树→AST→IR→汇编 + 优化
§3  Linux+JRE · cbc hello.cb → ./hello
```

---

## 三句背诵

1. **cbc 把 C♭ 编译成 x86 Linux 上的 ELF 可执行文件。**
2. **Build 四步管全链；cbc 内四步管狭义编译。**
3. **语法→语义→IR→代码生成，对应全书四大部分主体。**

---

## 对照表

| 术语 | 一句话 |
|------|--------|
| C♭ | 含指针的 C 子集 |
| ELF | Linux 可执行/目标文件格式 |
| 语法树 | 文法结构 |
| AST | 带语义的结构树 |
| IR | 编译器内部中间表示 |

---

## 自测

- [ ] Build 四阶段名称与顺序
- [ ] cbc 内四阶段与本书第几部分对应
- [ ] 语法树与 AST 区别
- [ ] `cbc hello.cb` 产出什么、如何运行

---

## 阅读进度

- [x] ch1 开始制作编译器
- [x] ch2 C♭ 和 cbc

