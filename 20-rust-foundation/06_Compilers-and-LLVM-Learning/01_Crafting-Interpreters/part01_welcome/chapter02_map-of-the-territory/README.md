# 第 2 章 · A Map of the Territory（领域地图）

> **Crafting Interpreters** · [Part I · Welcome](../README.md) · [本书目录](../../本书目录.md)  
> 在线：[craftinginterpreters.com](https://craftinginterpreters.com/a-map-of-the-territory.html) · [中文在线](https://craftinginterpreters.com/a-map-of-the-territory.html)

## 状态

- [x] 已读（笔记整理）

---

## 一句话

第二章仍不写实现代码，而是给出高层架构图：从人类可读的源代码到机器执行，中间要经过哪些阶段。作者用 「攀登山峰再下山」 比喻整条流水线（与封面「编译之山」一致）。

---

## 专项笔记（一小节一文件）

| 小节 | 主题 | 阅读 |
|------|------|------|
| — | 本章定位 | [00-overview.md](./00-overview.md) |
| §2.1 | 语言的组成部分（The Parts of a Language） | [01-the-parts-of-a-language.md](./01-the-parts-of-a-language.md) |
| §2.2 | 捷径与替代方案（Shortcuts and Alternate Routes） | [02-shortcuts-and-alternate-routes.md](./02-shortcuts-and-alternate-routes.md) |
| ·4 | 「编译之山」与本书两趟实现 | [03-a-map-of-the-territory.md](./03-a-map-of-the-territory.md) |
| 延伸 | Rust/HFT · 上山前端 + 下山后端分层 | [04-rust-hft-编译流水线对照.md](./04-rust-hft-编译流水线对照.md) |
| 延伸 | **编译期 LLVM vs 运行期 Runtime**（彻底分开） | [05-compile-time-llvm-vs-runtime.md](./05-compile-time-llvm-vs-runtime.md) |
| — | 速记 · 自测 · 进度 |

---

## 逻辑脉络


---

## 速记

## 本章速记

```text
§2.1  扫描→Token · 解析→AST · 静态分析 · IR · 优化 · 代码生成 · VM/字节码 · Runtime/GC
§2.2  单遍（省 AST）· 树遍历（jlox）· 转译（换高级语言）
全书  = 先 §2.2.2 再 §2.1.7 的 clox 路线
Rust  = 源码→AST→LLVM IR→机器码→ELF（见 04-rust-hft-编译流水线对照.md）
LLVM≠Runtime = 编译期翻译 vs 运行期调度（见 05-compile-time-llvm-vs-runtime.md）
```

---

---

## 读后下一步

| 章 | 目录 | 内容 |
|:--:|------|------|
| **3** | [chapter03 · The Lox Language](../../chapter03_the-lox-language/) | **Lox 语言规格**——动手写 jlox 前必读 |
| **4** | Part II · Scanning | 流水线第一站：Token |

---

---

## 自测 / 对照（可选）

- [ ] 画一条从「Rust 源码」到「CPU 执行」的简化 pipeline（可含 `rustc` → LLVM IR → 机器码）。
- [ ] 说明 **jlox** 在 §2.1 的哪几站停下、**clox** 多走了哪几站。
- [ ] 各举 1 个 **transpiler** 与 **VM 语言** 的例子。
- [ ] 对照 RFR [03-2 OS/LLVM 内存布局](../../../../02-RFR/Chapter-01-Foundations/03-2-os-memory-layout.md)：`alloca`（栈）vs heap 分别更像 pipeline 哪一段的产物？
- [ ] 一句话区分 **LLVM**（编译期）与 **Runtime**（运行期）；Tokio 属于哪一类 runtime？

---

---

## 阅读进度

- [x] §2.1～§2.2 结构梳理（本章笔记）
- [ ] 本章 Challenges（若有）
- [ ] 进入 ch03 Lox 规格

