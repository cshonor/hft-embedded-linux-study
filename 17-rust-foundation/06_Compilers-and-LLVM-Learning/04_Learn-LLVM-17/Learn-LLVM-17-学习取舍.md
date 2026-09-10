# 《Learn LLVM 17》13 章：零基础精简解读 + 学习取舍

全书 4 大板块 13 章。**目标**：服务 **Rust 底层、并发、性能**；**不必**全书通读；**不必**在本仓写 C++ 编译器。

**与仓库文件夹对齐**：见 **[README.md](./README.md)**；精读/浏览/跳过与下表一致。

---

## 开读本书前的 C++ 前置（必修）

LLVM 用 **C++** 实现；《Learn LLVM 17》示例与 API 默认你已具备 **C++11/14** 阅读能力。

> **更正（2026-09-10）**：此处原要求去姊妹仓 `cpp-learning-notes` 读 `01`～`06` —— **已过时**。
> 那些笔记**早已复制进本仓 `04-cpp`**（M0–M5，751 篇）。且 LLVM 用**非典型 C++**（禁用异常 / RTTI、自研 ADT），
> **不必通读 6 本**，按下表取最小子集即可（约 15–20 小时，是复习不是从零学）。

| 需要 | 本仓入口 |
|------|----------|
| 类 · 拷贝控制 | [`04-cpp/M0/…/ch07-classes`](../../../04-cpp/M0-entry-syntax/01-C%2B%2BPrimer/ch07-classes/) · [`ch13-copy-control`](../../../04-cpp/M0-entry-syntax/01-C%2B%2BPrimer/ch13-copy-control/) |
| 模板基础 | [`04-cpp/M0/…/ch16-templates`](../../../04-cpp/M0-entry-syntax/01-C%2B%2BPrimer/ch16-templates/) |
| 现代 C++ | [`04-cpp/M1/01-Effective-Modern-C++`](../../../04-cpp/M1-modern-cpp/01-Effective-Modern-C%2B%2B/) |

**可跳过**：`Effective STL`、`STL 源码剖析`（LLVM 用自研 ADT 而非 `std::` 容器）、并发、C++17/20。

完整清单 → [`20-compilers-llvm/_refs/cpp-minimum-for-llvm.md`](../../../20-compilers-llvm/_refs/cpp-minimum-for-llvm.md)；总路线 → [05/README](../README.md)。

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
