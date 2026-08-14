# 第 1 章 · 编译总览（Overview of Compilation）

> **Engineering a Compiler 3e** · [02 编译器工程](../../README.md) · [本书目录](../../本书目录.md)

## 状态

- [x] 已读（笔记整理）

---

## 一句话

第一章不写具体算法，而是搭建**编译器工程总框架**：编译器是什么、必须守什么原则、典型三阶段结构、一次翻译经历哪些步骤、好编译器还应具备什么工程性质——本质是**正确性、实用性与各种 Trade-off 之间的权衡艺术**。

---

## 专项笔记（一小节一文件）

| 小节 | 主题 | 阅读 |
|------|------|------|
| — | 本章定位 | [00-overview.md](./00-overview.md) |
| §1 | 基本概念与研究动机 | [01-basic-concepts-and-motivation.md](./01-basic-concepts-and-motivation.md) |
| §2 | 两大基本原则 | [02-two-fundamental-principles.md](./02-two-fundamental-principles.md) |
| §3 | 三阶段结构（前端 / 优化器 / 后端） | [03-three-phase-structure.md](./03-three-phase-structure.md) |
| §4 | 翻译过程关键步骤（含示例语句） | [04-translation-pipeline-example.md](./04-translation-pipeline-example.md) |
| §5 | 编译器应有的工程性质 | [05-desired-properties.md](./05-desired-properties.md) |
| — | 速记 · 自测 · 对照 |

---

## 与仓库其他部分

| 本书 ch1 | 对照 |
|----------|------|
| 三阶段 / IR | [CI ch2 · Rust/HFT 流水线](../../01_Crafting-Interpreters/part01_welcome/chapter02_map-of-the-territory/04-rust-hft-编译流水线对照.md) |
| 优化 / 寄存器分配 | [04 Learn LLVM 17](../../../04_Learn-LLVM-17/README.md) |
| Rust 编译链 | RFR [05 编译与分发](../../../../02-RFR/Chapter-02-Types/05-compilation-dispatch.md) |

---

## 逻辑脉络


---

## 速记

## 本章速记

```text
§1  编译器=翻译器（产物需再运行）· 解释器=直接执行 · 大图算法+启发式
§2  正确性（不改语义）+ 实用性（明确改进）
§3  前端→IR · 优化器→改进IR · 后端→机器码
§4  理解输入·运行环境·优化·代码生成（指令筛选/寄存器分配/指令调度）
§5  目标程序快/小 · 编译器自身也要快/省内存 · 好诊断·可调试
本质 = Trade-offs 的艺术与科学
```

---

## 三句背诵

1. **编译器翻译、解释器执行；正确性是不改语义的契约。**
2. **三阶段：前端理解→IR，优化器改 IR，后端映射机器。**
3. **后端三板斧：选指令、分寄存器、排顺序。**

---

## 与 CI / Rust 对照

| 橡书 ch1 | 本仓库 |
|----------|--------|
| 三阶段 | [CI ch2 · 04-rust-hft-流水线](../../01_Crafting-Interpreters/part01_welcome/chapter02_map-of-the-territory/04-rust-hft-编译流水线对照.md) |
| 优化 / IR | [04 LLVM](../../../04_Learn-LLVM-17/README.md) |
| 寄存器 / 调度 | RFR 第 2 章 · HFT 延迟 |

---

## 自测

- [ ] 编译器与解释器的核心区别（一句话）
- [ ] 两大基本原则各是什么
- [ ] 画出三阶段结构，各写一句职责
- [ ] 后端代码生成的三个难点
- [ ] 举 1 个「正确性 vs 实用性」的 Trade-off（如 `-O0` / `-O3`）

---

## 阅读进度

- [x] ch1 编译总览（本章笔记）
- [ ] ch2 及以后（待读）

