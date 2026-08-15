# 03 · Deep Rust StdLib（标准库进阶）

> 原书：《深入 Rust 标准库：必备的 Rust 语言高级指南》— 完整 PDF 目录见 **[本书目录.md](./本书目录.md)**  
> Rust 主线：**`00-Book` → `02-RFR` → `01-ER` → 本目录** → [`04-Rust-Nomicon`](../04-Rust-Nomicon/README.md) → [`05-Async-Concurrency-Network`](../05-Async-Concurrency-Network/README.md)

在 **Effective Rust** 之后、**Nomicon** 之前，按原书章节啃 **标准库底层与内存操作** — 为 unsafe / HFT 内存池 / 并发专题打底。

**笔记结构**：每章文件夹 + **每小节一个 `.md`**（README 仅为索引）；新建节默认为可填充模板，刷书 / 对照源码后迭代。

---

## 阅读定位

| 阶段 | 仓库 | 侧重 |
|------|------|------|
| 语法底座 | [`00-Book`](../00-Book/Book-本书目录.md) | 所有权、类型、错误、trait |
| 内功 | [`02-RFR`](../02-RFR/RFR-本书目录.md) | 内存模型、类型系统、异步原理 |
| 工程习惯 | [`01-ER`](../01-ER/ER-本书目录.md) | API 设计、惯用写法 |
| **标准库进阶** | **本目录** | `std` 模块语义、容器/IO/并发原语、与源码对照 |
| unsafe 边界 | [`04-Rust-Nomicon`](../04-Rust-Nomicon/README.md) | 布局、型变、未初始化、FFI |
| 实战 | [`05-Async-Concurrency-Network`](../05-Async-Concurrency-Network/README.md) | atomic · tokio · 网络 |

---

## 目录（原书对照）

| 章 | 原书主题 | 入口 | 进度 |
|:---:|----------|------|------|
| **1** | 标准库体系概述（1.1～1.4） | [chapter01_std_overview/](./chapter01_std_overview/README.md) | ✅ 正文 |
| **2** | Rust 特征小议（2.1～2.4） | [chapter02_rust_features_summary/](./chapter02_rust_features_summary/README.md) | ✅ 正文 |
| **3** | **内存操作**（3.1～3.11） | [chapter03_memory_model/](./chapter03_memory_model/README.md) | 📝 核心节 ✅ · 3.1.2～3.2.6 等待刷书 |
| **4** | 基本类型及基本 Trait（4.1～4.3） | [chapter04_primitive_types/](./chapter04_primitive_types/README.md) | ✅ 正文 |
| **5** | 迭代器（5.1～5.9） | [chapter05_iterators/](./chapter05_iterators/README.md) | ✅ 正文 |
| **6** | 基本类型（续）（6.1～6.5） | [chapter06_basic_types_continued/](./chapter06_basic_types_continued/README.md) | ✅ 正文 |
| **7** | 内部可变性类型（7.1～7.5） | [chapter07_interior_mutability/](./chapter07_interior_mutability/README.md) | ✅ 正文 |
| **8** | 智能指针（8.1～8.8） | [chapter08_smart_pointers/](./chapter08_smart_pointers/README.md) | 📝 8.1～8.5 ✅ · 8.6～8.8 待刷书 |
| **9** | 用户态标准库基础（9.1～9.6） | [chapter09_userspace_std_basics/](./chapter09_userspace_std_basics/README.md) | ✅ 正文 |
| **10** | 进程管理（10.1～10.3 + 10.4 unsafe 走读） | [chapter10_process_management/](./chapter10_process_management/README.md) | ✅ 补充解析 |
| **11** | 并发编程（11.0～11.13） | [chapter11_concurrency/](./chapter11_concurrency/README.md) | ✅ 补充解析 |
| **12** | 文件系统（12.0～12.5） | [chapter12_filesystem/](./chapter12_filesystem/README.md) | ✅ 补充解析 |
| **13** | I/O 系统（13.0～13.5） | [chapter13_io/](./chapter13_io/README.md) | ✅ 补充解析 |
| **14** | 异步编程（14.0～14.5 · 全书压轴） | [chapter14_async/](./chapter14_async/README.md) | ✅ 补充解析 |

细目 → **[本书目录.md](./本书目录.md)** · 批量生成/更新骨架：`python scripts/scaffold-deep-stdlib-notes.py`

**补充轨道**（非原书编号）：ch2 `2.x` 桥梁笔记与附录；ch3 [_supplement/](./chapter03_memory_model/_supplement/README.md) 旧编号笔记。

---

## 相关

- 纯阅读路线 → [`docs/纯阅读路线.md`](../docs/纯阅读路线.md)
- 仓库总索引 → [`README.md`](../README.md)
