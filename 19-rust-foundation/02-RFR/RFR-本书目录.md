# 《Rust for Rustaceans》目录

**Rust for Rustaceans: Idiomatic Programming for Experienced Developers**

> 与仓库内 `02-RFR/Chapter-01-Foundations/` … `Chapter-13-Rust-Ecosystem/` 文件夹一一对应，可按章做笔记或放 demo。

**何时读、如何配合 network/async/locks/LLVM/TLPI**：见 **[学习路径与章节对照.md](./学习路径与章节对照.md)**（含 13 章 ↔ 仓库板块对照表与问题驱动速查）。

---

## 前言与致谢

- Foreword, Preface, Acknowledgments

## 引言

- Introduction

---

## 第 1 章：基础 (Foundations) — `Chapter-01-Foundations/`

**4 个主节**（含二级子节共 **11 个部分**）：

| 主节 | 子节 / 笔记 |
|------|-------------|
| **Talking About Memory** | [01 内存术语](Chapter-01-Foundations/01-memory-terminology.md) · [02 变量深入](Chapter-01-Foundations/02-variables-in-depth.md) · [03 内存区域](Chapter-01-Foundations/03-memory-regions.md)（[03.1 Rust](Chapter-01-Foundations/03-1-rust-memory-model.md) · [03.2 OS/LLVM](Chapter-01-Foundations/03-2-os-memory-layout.md)） |
| **Ownership** | [04 所有权](Chapter-01-Foundations/04-ownership.md) |
| **Borrowing and Lifetimes** | [05 共享引用](Chapter-01-Foundations/05-shared-references.md) · [06 可变引用](Chapter-01-Foundations/06-mutable-references.md) · [07 内部可变性](Chapter-01-Foundations/07-interior-mutability.md) · [08 生命周期](Chapter-01-Foundations/08-lifetimes.md) |
| **Summary** | [09 小结](Chapter-01-Foundations/09-summary.md) |

章索引：[Chapter-01-Foundations/README.md](Chapter-01-Foundations/README.md)

## 第 2 章：类型 (Types) — `Chapter-02-Types/`

**4 个主节**（含二级子节共 **13 个部分**）：

| 主节 | 子节 / 笔记 |
|------|-------------|
| **Types in Memory** | [01 对齐](Chapter-02-Types/01-alignment.md) · [02 布局](Chapter-02-Types/02-layout.md) · [03 复合类型](Chapter-02-Types/03-complex-types.md) · [04 DST 与宽指针](Chapter-02-Types/04-dst-wide-pointers.md)（[04.1](Chapter-02-Types/04-1-dst-basics.md)～[04.4](Chapter-02-Types/04-4-containers-ffi-hft.md)） |
| **Traits and Trait Bounds** | [05 编译与分发](Chapter-02-Types/05-compilation-dispatch.md)（[05.1](Chapter-02-Types/05-1-static-vs-dynamic.md)～[05.4](Chapter-02-Types/05-4-selection-hft.md)）· [06 泛型 Trait](Chapter-02-Types/06-generic-traits.md)（[06.1](Chapter-02-Types/06-1-associated-vs-generic.md)～[06.2](Chapter-02-Types/06-2-existential-hft.md)）· [07 相干性与孤儿规则](Chapter-02-Types/07-coherence-orphan-rule.md)（[07.1](Chapter-02-Types/07-1-orphan-rule.md)～[07.3](Chapter-02-Types/07-3-newtype-practice.md)）· [08 Trait 限定](Chapter-02-Types/08-trait-bounds.md)（[08.1](Chapter-02-Types/08-1-syntax-static-dynamic.md)～[08.3](Chapter-02-Types/08-3-examples-pitfalls.md)）· [09 标记 Trait](Chapter-02-Types/09-marker-traits.md)（[09.1](Chapter-02-Types/09-1-marker-basics.md)～[09.4](Chapter-02-Types/09-4-unsafe-impl.md)） |
| **Existential Types** | [10 存在类型](Chapter-02-Types/10-existential-types.md)（[10.1](Chapter-02-Types/10-1-logic-positions.md)～[10.3](Chapter-02-Types/10-3-limits-selection.md)） |
| **Summary** | [11 小结](Chapter-02-Types/11-summary.md) |

章索引：[Chapter-02-Types/README.md](Chapter-02-Types/README.md)

## 第 3 章：接口设计 (Designing Interfaces) — `Chapter-03-Designing-Interfaces/`

**5 个主节**（含二级子节共 **18 个部分**）：

| 主节 | 子节 / 笔记 |
|------|-------------|
| **Unsurprising** | [01 命名](Chapter-03-Designing-Interfaces/01-naming-practices.md) · [02 通用 Trait](Chapter-03-Designing-Interfaces/02-common-traits-for-types.md) · [03 人体工程学 impl](Chapter-03-Designing-Interfaces/03-ergonomic-trait-implementations.md) · [04 包装类型](Chapter-03-Designing-Interfaces/04-wrapper-types.md) |
| **Flexible** | [05 泛型参数](Chapter-03-Designing-Interfaces/05-generic-arguments.md) · [06 对象安全](Chapter-03-Designing-Interfaces/06-object-safety.md) · [07 借用 vs 拥有](Chapter-03-Designing-Interfaces/07-borrowed-vs-owned.md) · [08 可失败析构](Chapter-03-Designing-Interfaces/08-fallible-blocking-destructors.md) |
| **Obvious** | [09 文档](Chapter-03-Designing-Interfaces/09-documentation.md) · [10 类型引导](Chapter-03-Designing-Interfaces/10-type-system-guidance.md) |
| **Constrained** | [11 类型演进](Chapter-03-Designing-Interfaces/11-type-modifications.md) · [12 Trait 控制](Chapter-03-Designing-Interfaces/12-trait-implementations.md) · [13 隐藏契约](Chapter-03-Designing-Interfaces/13-hidden-contracts.md) |
| **Summary** | [14 小结](Chapter-03-Designing-Interfaces/14-summary.md) |

章索引：[Chapter-03-Designing-Interfaces/README.md](Chapter-03-Designing-Interfaces/README.md)

## 第 4 章：错误处理 (Error Handling) — `Chapter-04-Error-Handling/`

**3 个主节**（含二级子节共 **6 个部分**）：

| 主节 | 子节 / 笔记 |
|------|-------------|
| **Representing Errors** | [01 枚举](Chapter-04-Error-Handling/01-enumeration.md) · [02 不透明](Chapter-04-Error-Handling/02-opaque-errors.md) · [03 特殊情形](Chapter-04-Error-Handling/03-special-error-cases.md) |
| **Propagating Errors** | [04 传播](Chapter-04-Error-Handling/04-propagating-errors.md) |
| **Summary** | [05 小结](Chapter-04-Error-Handling/05-summary.md) |

章索引：[Chapter-04-Error-Handling/README.md](Chapter-04-Error-Handling/README.md)

## 第 5 章：项目结构 (Project Structure) — `Chapter-05-Project-Structure/`

**6 个主节**（含二级子节共 **14 个部分**）：

| 主节 | 子节 / 笔记 |
|------|-------------|
| **Features** | [01 定义与包含](Chapter-05-Project-Structure/01-defining-including-features.md) · [02 crate 内使用](Chapter-05-Project-Structure/02-using-features-in-crate.md) |
| **Workspaces** | [03 工作区](Chapter-05-Project-Structure/03-workspaces.md) |
| **Project Configuration** | [04 元数据](Chapter-05-Project-Structure/04-crate-metadata.md) · [05 构建配置](Chapter-05-Project-Structure/05-build-configuration.md) |
| **Conditional Compilation** | [06 条件编译](Chapter-05-Project-Structure/06-conditional-compilation.md) |
| **Versioning** | [07 MSRV](Chapter-05-Project-Structure/07-msrv.md) · [08 依赖下界](Chapter-05-Project-Structure/08-minimal-dependency-versions.md) · [09 Changelog](Chapter-05-Project-Structure/09-changelogs.md) · [10 未发布版本](Chapter-05-Project-Structure/10-unreleased-versions.md) |
| **Summary** | [11 小结](Chapter-05-Project-Structure/11-summary.md) |

章索引：[Chapter-05-Project-Structure/README.md](Chapter-05-Project-Structure/README.md)

## 第 6 章：测试 (Testing) — `Chapter-06-Testing/`

**3 个主节**（含二级子节共 **10 个部分**）：

| 主节 | 子节 / 笔记 |
|------|-------------|
| **Rust Testing Mechanisms** | [01 脚手架](Chapter-06-Testing/01-test-harness.md) · [02 cfg(test)](Chapter-06-Testing/02-cfg-test.md) · [03 doctest](Chapter-06-Testing/03-doctests.md) |
| **Additional Testing Tools** | [04 Lint](Chapter-06-Testing/04-linting.md) · [05 测试生成](Chapter-06-Testing/05-test-generation.md) · [06 测试增强](Chapter-06-Testing/06-test-augmentation.md) · [07 性能测试](Chapter-06-Testing/07-performance-testing.md) |
| **Summary** | [08 小结](Chapter-06-Testing/08-summary.md) |

章索引：[Chapter-06-Testing/README.md](Chapter-06-Testing/README.md)

## 第 7 章：宏 (Macros) — `Chapter-07-Macros/`

**3 个主节**（含二级子节共 **10 个部分**）：

| 主节 | 子节 / 笔记 |
|------|-------------|
| **Declarative Macros** | [01 何时使用](Chapter-07-Macros/01-when-to-use-declarative-macros.md) · [02 如何工作](Chapter-07-Macros/02-how-declarative-macros-work.md) · [03 如何编写](Chapter-07-Macros/03-how-to-write-declarative-macros.md) |

总览：[00 宏核心总览](Chapter-07-Macros/00-macros-overview.md) · 动手：[proc-macro-workshop 实验](Chapter-07-Macros/proc-macro-workshop-lab.md)
| **Procedural Macros** | [04 过程宏类型](Chapter-07-Macros/04-types-of-procedural-macros.md) · [05 代价](Chapter-07-Macros/05-cost-of-procedural-macros.md) · [06 你真的需要宏吗](Chapter-07-Macros/06-so-you-think-you-want-a-macro.md) · [07 如何工作](Chapter-07-Macros/07-how-procedural-macros-work.md) |
| **Summary** | [08 小结](Chapter-07-Macros/08-summary.md) |

章索引：[Chapter-07-Macros/README.md](Chapter-07-Macros/README.md) · 动手：[proc-macro-workshop 实验](Chapter-07-Macros/proc-macro-workshop-lab.md)

## 第 8 章：异步编程 (Asynchronous Programming) — `Chapter-08-Asynchronous-Programming/`

**5 个主节**（含二级子节共 **15 个部分**）：

| 主节 | 子节 / 笔记 |
|------|-------------|
| **What's the Deal with Asynchrony?** | [01 同步](Chapter-08-Asynchronous-Programming/01-synchronous-interfaces.md) · [02 多线程](Chapter-08-Asynchronous-Programming/02-multithreading.md) · [03 异步接口](Chapter-08-Asynchronous-Programming/03-asynchronous-interfaces.md) · [04 轮询](Chapter-08-Asynchronous-Programming/04-standardized-polling.md) |
| **Ergonomic Futures** | [05 async/await](Chapter-08-Asynchronous-Programming/05-async-await.md) · [06 Pin/Unpin](Chapter-08-Asynchronous-Programming/06-pin-unpin.md) |
| **Going to Sleep** | [07 唤醒](Chapter-08-Asynchronous-Programming/07-waking-up.md) · [08 Poll 契约](Chapter-08-Asynchronous-Programming/08-poll-contract.md) · [09 误称](Chapter-08-Asynchronous-Programming/09-waking-misnomer.md) · [10 任务](Chapter-08-Asynchronous-Programming/10-tasks-subexecutors.md) |
| **spawn** | [11 spawn](Chapter-08-Asynchronous-Programming/11-spawn.md) |
| **Summary** | [12 小结](Chapter-08-Asynchronous-Programming/12-summary.md) |

章索引：[Chapter-08-Asynchronous-Programming/README.md](Chapter-08-Asynchronous-Programming/README.md)

## 第 9 章：不安全代码 (Unsafe Code) — `Chapter-09-Unsafe-Code/`

**5 个主节**（含二级子节共 **16 个部分**）：

| 主节 | 子节 / 笔记 |
|------|-------------|
| **The unsafe Keyword** | [01 关键字](Chapter-09-Unsafe-Code/01-unsafe-keyword.md) |
| **Great Power** | [02 裸指针](Chapter-09-Unsafe-Code/02-raw-pointers.md) · [03 调用 unsafe](Chapter-09-Unsafe-Code/03-calling-unsafe-functions.md) · [04 unsafe trait](Chapter-09-Unsafe-Code/04-unsafe-traits.md) |
| **Great Responsibility** | [05–09 责任](Chapter-09-Unsafe-Code/05-what-can-go-wrong.md)（含 Validity · Panic · Casting · Drop） |
| **Coping with Fear** | [10 边界](Chapter-09-Unsafe-Code/10-manage-boundaries.md) · [11 文档](Chapter-09-Unsafe-Code/11-documentation.md) · [12 验证](Chapter-09-Unsafe-Code/12-check-your-work.md) |
| **Summary** | [13 小结](Chapter-09-Unsafe-Code/13-summary.md) |

章索引：[Chapter-09-Unsafe-Code/README.md](Chapter-09-Unsafe-Code/README.md)

## 第 10 章：并发与并行 (Concurrency and Parallelism) — `Chapter-10-Concurrency-and-Parallelism/`

**6 个主节**（含二级子节共 **19 个部分**）：

| 主节 | 子节 / 笔记 |
|------|-------------|
| **The Trouble with Concurrency** | [01 正确性](Chapter-10-Concurrency-and-Parallelism/01-correctness.md) · [02 性能](Chapter-10-Concurrency-and-Parallelism/02-performance.md) |
| **Concurrency Models** | [03 共享内存](Chapter-10-Concurrency-and-Parallelism/03-shared-memory.md) · [04 工作池](Chapter-10-Concurrency-and-Parallelism/04-worker-pools.md) · [05 Actor](Chapter-10-Concurrency-and-Parallelism/05-actors.md) |
| **Asynchrony and Parallelism** | [06 异步与并行](Chapter-10-Concurrency-and-Parallelism/06-asynchrony-parallelism.md) |
| **Lower-Level Concurrency** | [07–11 原子](Chapter-10-Concurrency-and-Parallelism/07-memory-operations.md)（含 ordering · CAS · fetch） |
| **Sane Concurrency** | [12 简单](Chapter-10-Concurrency-and-Parallelism/12-start-simple.md) · [13 压测](Chapter-10-Concurrency-and-Parallelism/13-stress-tests.md) · [14 工具](Chapter-10-Concurrency-and-Parallelism/14-concurrency-testing-tools.md) |
| **Summary** | [15 小结](Chapter-10-Concurrency-and-Parallelism/15-summary.md) |

章索引：[Chapter-10-Concurrency-and-Parallelism/README.md](Chapter-10-Concurrency-and-Parallelism/README.md)

## 第 11 章：外部函数接口 (Foreign Function Interfaces) — `Chapter-11-Foreign-Function-Interfaces/`

**4 个主节**（含二级子节共 **10 个部分**）：

| 主节 | 子节 / 笔记 |
|------|-------------|
| **Crossing Boundaries with extern** | [01 符号](Chapter-11-Foreign-Function-Interfaces/01-symbols.md) · [02 调用约定](Chapter-11-Foreign-Function-Interfaces/02-calling-conventions.md) |
| **Types Across Language Boundaries** | [03 类型](Chapter-11-Foreign-Function-Interfaces/03-type-matching.md) · [04 分配](Chapter-11-Foreign-Function-Interfaces/04-allocations.md) · [05 回调](Chapter-11-Foreign-Function-Interfaces/05-callbacks.md) · [06 安全](Chapter-11-Foreign-Function-Interfaces/06-safety.md) |
| **bindgen and Build Scripts** | [07 bindgen](Chapter-11-Foreign-Function-Interfaces/07-bindgen-build-scripts.md) |
| **Summary** | [08 小结](Chapter-11-Foreign-Function-Interfaces/08-summary.md) |

章索引：[Chapter-11-Foreign-Function-Interfaces/README.md](Chapter-11-Foreign-Function-Interfaces/README.md)

## 第 12 章：脱离标准库的 Rust (Rust Without the Standard Library) — `Chapter-12-Rust-Without-Standard-Library/`

**7 个主节**（含二级子节共 **10 个部分**）：

| 主节 | 子节 / 笔记 |
|------|-------------|
| **Opting Out of std** | [01 no_std](Chapter-12-Rust-Without-Standard-Library/01-opting-out-no-std.md) |
| **Dynamic Memory Allocation** | [02 分配](Chapter-12-Rust-Without-Standard-Library/02-dynamic-memory-allocation.md) |
| **The Rust Runtime** | [03 Panic](Chapter-12-Rust-Without-Standard-Library/03-panic-handler.md) · [04 初始化](Chapter-12-Rust-Without-Standard-Library/04-program-initialization.md) · [05 OOM](Chapter-12-Rust-Without-Standard-Library/05-oom-handler.md) |
| **Low-Level Memory Accesses** | [06 volatile](Chapter-12-Rust-Without-Standard-Library/06-low-level-memory-access.md) |
| **Hardware Abstraction** | [07 类型状态 HAL](Chapter-12-Rust-Without-Standard-Library/07-hardware-abstraction.md) |
| **Cross-Compilation** | [08 交叉编译](Chapter-12-Rust-Without-Standard-Library/08-cross-compilation.md) |
| **Summary** | [09 小结](Chapter-12-Rust-Without-Standard-Library/09-summary.md) |

章索引：[Chapter-12-Rust-Without-Standard-Library/README.md](Chapter-12-Rust-Without-Standard-Library/README.md)

## 第 13 章：Rust 生态系统 (The Rust Ecosystem) — `Chapter-13-Rust-Ecosystem/`

**5 个主节**（含二级子节共 **17 个部分**）：

| 主节 | 子节 / 笔记 |
|------|-------------|
| **What's Out There?** | [01 工具](Chapter-13-Rust-Ecosystem/01-tools.md) · [02 库](Chapter-13-Rust-Ecosystem/02-libraries.md) · [03 工具链](Chapter-13-Rust-Ecosystem/03-rust-tooling.md) · [04 标准库](Chapter-13-Rust-Ecosystem/04-standard-library.md) |
| **Patterns in the Wild** | [05 索引指针](Chapter-13-Rust-Ecosystem/05-index-pointers.md) · [06 Drop 卫士](Chapter-13-Rust-Ecosystem/06-drop-guards.md) · [07 扩展 Trait](Chapter-13-Rust-Ecosystem/07-extension-traits.md) · [08 prelude](Chapter-13-Rust-Ecosystem/08-crate-preludes.md) |
| **Staying Up to Date** | [09 保持更新](Chapter-13-Rust-Ecosystem/09-staying-up-to-date.md) |
| **What Next?** | [10 看](Chapter-13-Rust-Ecosystem/10-learn-by-watching.md) · [11 做](Chapter-13-Rust-Ecosystem/11-learn-by-doing.md) · [12 读](Chapter-13-Rust-Ecosystem/12-learn-by-reading.md) · [13 教](Chapter-13-Rust-Ecosystem/13-learn-by-teaching.md) |
| **Summary** | [14 小结](Chapter-13-Rust-Ecosystem/14-summary.md) |

章索引：[Chapter-13-Rust-Ecosystem/README.md](Chapter-13-Rust-Ecosystem/README.md)

---

## 索引 (Index)

原书全书末尾术语索引（**无子节**，约第 245 页）→ **[Index.md](Index.md)**
