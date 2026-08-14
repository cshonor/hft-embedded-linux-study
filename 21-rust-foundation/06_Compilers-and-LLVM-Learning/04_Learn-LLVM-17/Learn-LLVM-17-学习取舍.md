# 《Learn LLVM 17》13 章：零基础精简解读 + 学习取舍

全书 4 大板块 13 章。**目标**：服务 **Rust 底层、并发、性能**；**不必**全书通读；**不必**在本仓写 C++ 编译器。

**与仓库文件夹对齐**：见 **[README.md](./README.md)**；精读/浏览/跳过与下表一致。

---

## 开读本书前的 C++ 前置（必修）

LLVM 用 **C++** 实现；《Learn LLVM 17》示例与 API 默认你已具备 **C++11/14 + STL** 阅读能力。  
请在 **[cpp-learning-notes](https://github.com/cshonor/cpp-learning-notes)** 读完 **`01`～`06`** 后再开本目录：

| # | 目录 | 书名 |
|---|------|------|
| 01 | `01-C++Primer` | C++ Primer |
| 02 | `02-Effective-C++` | Effective C++ |
| 03 | `03-More-Effective-C++` | More Effective C++ |
| 04 | `04-Effective-Modern-C++` | Effective Modern C++ |
| 05 | `05-Effective-STL` | Effective STL |
| 06 | `06-STL-Source-Analysis` | STL 源码剖析 |

详表与总路线 → [05/README](../README.md)。**07～09**（对象模型、并发、C++20）可与 Rust `04` 并行，不挡 LLVM 入门。

---

## 新手最简清单

### 必须精读

第 2、4、5、7、10 章

### 浏览即可

第 1、3、6、9 章

### 整段跳过

第 8、11、12、13 章

---

## 与当前仓库学习路线的贴合顺序

1. **[04/01-atomic/](../../05-Async-Concurrency-Network/01-atomic/)**：原子、内存序、锁  
2. **[04/02-async_tokio/](../../05-Async-Concurrency-Network/02-async_tokio/)**  
3. **[04/03-rust_network_programming/](../../05-Async-Concurrency-Network/03-rust_network_programming/)**  
4. **本目录按上表精读**：用前面 Rust 代码**反查 IR 与优化**

---

## 进阶 C++（07～09，与 Rust 04 并行）

| C++（cpp-learning-notes） | 本仓库 Rust | 用途 |
|---------------------------|-------------|------|
| `07-Cpp-Object-Model` | RFR 第 2 章 · Nomicon | struct / vtable ↔ IR 第 4～5 章 |
| `08-Cpp-Concurrency` | `04/01-atomic` | `ir_samples/atomic_ir/` |
| `09-C++20-The-Complete-Guide` | Rust 2021 特性 | 协程等概念对照（非 LLVM 必修） |
| 可选 `11-Modern-C++-Performance-Engineering` | 04 + 本书 ch07 | O0/O3、`optimize_compare/` |

---

## 速记

**精读** 2 · 4 · 5 · 7 · 10 · **浏览** 1 · 3 · 6 · 9 · **跳过** 8 · 11–13  
**IR 主线**：改 `llvm_insight_lab` → `emit=llvm-ir` → 片段进 `ir_samples/`
