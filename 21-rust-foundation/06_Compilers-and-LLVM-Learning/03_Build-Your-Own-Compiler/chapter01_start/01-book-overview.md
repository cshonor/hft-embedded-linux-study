# 第 1 章 · 开始制作编译器 · §1 本书的概要

← [本章目录](./README.md) · 上一节：[00-overview.md](./00-overview.md) · 下一节：[02-four-compiler-stages.md](./02-four-compiler-stages.md)

---

## 核心目标

| 项目 | 说明 |
|------|------|
| **主题** | 从头制作 **C♭** 语言的编译器 **cbc** |
| **C♭** | 作者为本书设计的 **C 子集** — 保留 **指针** 等 C 核心 |
| **实现** | **Java** + **JavaCC**（后续章节展开） |
| **产物** | 可直接运行的 **ELF 可执行文件** — 非字节码/解释器 |

---

## 目标平台

```text
源语言 C♭  →  cbc  →  x86 汇编  →  ELF  →  Linux 上执行
```

| 维度 | 选型 |
|------|------|
| **CPU** | **x86 系列**（IA-32） |
| **OS** | **Linux** |
| **可执行格式** | **ELF**（Executable and Linking Format） |

→ 第4部分 ch18～21 专讲 ELF、链接、加载；本章先建立名词。

---

## Build 四阶段（源码 → 可执行文件）

Linux 下 **gcc 式** 完整构建，与 cbc **内部** 四阶段不同层次：

```text
  .cb / .c 源码
       ↓
  ① 预处理      cpp：宏、#include
       ↓
  ② 狭义编译    编译器：源码 → 汇编（.s）
       ↓
  ③ 汇编        as：汇编 → 目标文件（.o）
       ↓
  ④ 链接        ld：.o + 库 → 可执行 ELF
```

| 阶段 | 工具（典型） | 输出 |
|------|--------------|------|
| 预处理 | `cpp` | 展开后的源码 |
| **狭义编译** | **cbc / gcc** | **汇编** |
| 汇编 | `as` | **目标文件** `.o` |
| 链接 | `ld` | **ELF 可执行** |

**cbc 的边界**：主要覆盖 ②；但书中 **cbc 一条龙** 常把 ②～④ 封装为 `cbc hello.cb` → `hello`（内部调用汇编器/链接器）。

---

## 与仓库对照

| 本书 | 对照 |
|------|------|
| ELF / 加载 | RFR [03-2 OS 内存布局](../../../../02-RFR/Chapter-01-Foundations/03-2-os-memory-layout.md) |
| 全链 | [CI ch2 下山后端](../../../01_Crafting-Interpreters/part01_welcome/chapter02_map-of-the-territory/04-rust-hft-编译流水线对照.md) |

---

## 自测

- [ ] C♭ 相对 C 的定位（子集 + 指针）
- [ ] Build 四阶段 vs cbc 内部四阶段（下一节）各指什么
- [ ] ELF 在哪一步出现
