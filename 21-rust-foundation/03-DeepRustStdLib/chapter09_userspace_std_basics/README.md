# 第 9 章 · 用户态标准库基础

> 所属：[03 DeepRustStdLib](../README.md) · 前：[第 8 章 智能指针](../chapter08_smart_pointers/README.md) · 后：[第 10 章 进程管理](../chapter10_process_management/README.md) · 原书目录：[本书目录 § 第 9 章](../本书目录.md#第-9-章--用户态标准库基础)

**本章定位**：**全书分水岭** — 从 **core/alloc（可无 OS）** 踏入 **`std` 用户态**。**FFI · System 堆 · SYSCALL · FD 所有权** 是后续进程/并发/文件/网络章的 **共同底座**。

**原书主线（已写入）**：9.1 C/OsString · 9.2 sys 分层 · 9.3 System 分配器 · 9.4 SYSCALL · 9.5 FD+Drop · 9.6→Ch10。

**阅读顺序**：**9.1 → 9.2 → … → 9.6**

---


<!-- AUTO:SECTION-INDEX -->

| 节 | 主题 | 笔记 |
|:---:|------|------|
| **9.1** | Rust 与 C 语言交互 | [笔记](./9.1-rust-c-interop.md) |
| **9.1.1** | C 语言的类型适配 | [笔记](./9.1.1-c-type-adaptation.md) |
| **9.1.2** | C 语言的 va_list 类型适配 | [笔记](./9.1.2-c-va-list.md) |
| **9.1.3** | C 语言字符串类型适配 | [笔记](./9.1.3-c-strings.md) |
| **9.1.4** | OsString 代码分析 | [笔记](./9.1.4-osstring.md) |
| **9.2** | 代码工程中的一个技巧 | [笔记](./9.2-engineering-trick.md) |
| **9.3** | 内存管理之 STD 库 | [笔记](./9.3-std-memory-mgmt.md) |
| **9.4** | 系统调用（SYSCALL）的封装 | [笔记](./9.4-syscall-wrapper.md) |
| **9.5** | 文件描述符及句柄 | [笔记](./9.5-fd-and-handles.md) |
| **9.5.1** | 文件描述符所有权设计 | [笔记](./9.5.1-fd-ownership.md) |
| **9.5.2** | 文件逻辑操作适配层 | [笔记](./9.5.2-fd-logic-layer.md) |
| **9.6** | 进程管理 | [笔记](./9.6-process-overview.md) |

<!-- /AUTO:SECTION-INDEX -->
## 子节索引

### 9.1 Rust 与 C 语言交互

| 节 | 主题 | 笔记 |
|:---:|------|------|
| **9.1** | Rust 与 C 语言交互 | ✅ |
| **9.1.1** | C 语言的类型适配 | ✅ |
| **9.1.2** | C 语言的 `va_list` 类型适配 | ✅ |
| **9.1.3** | C 语言字符串类型适配 | ✅ |
| **9.1.4** | `OsString` 代码分析 | ✅ |

### 9.2～9.6

| 节 | 主题 | 笔记 |
|:---:|------|------|
| **9.2** | 代码工程中的一个技巧 | ✅ sys/cfg |
| **9.3** | 内存管理之 STD 库 | ✅ System |
| **9.4** | 系统调用（SYSCALL）的封装 | ✅ |
| **9.5** | 文件描述符及句柄 | ✅ |
| **9.5.1** | 文件描述符所有权设计 | ✅ |
| **9.5.2** | 文件逻辑操作适配层 | ✅ |
| **9.6** | 进程管理 | ✅ 预告 Ch10 |

---

## 与主线对照

| 本章 | 本仓库延伸 |
|------|------------|
| FFI / C 类型 | [Nomicon 09 FFI](../../04-Rust-Nomicon/09_FFI/README.md) |
| `std` 三层 | [1.3 std 库](../chapter01_std_overview/1.3-std-crate.md) |
| 进程专章 | [第 10 章 进程管理](../chapter10_process_management/README.md) |
| 网络 I/O | [05-rust_network](../../05-Async-Concurrency-Network/03-rust_network_programming/README.md) |

---

## HFT 阅读提示

| 节 | 实盘关联 |
|----|----------|
| **9.1** | 对接 C 行情 SDK、交易 API |
| **9.5** | `File` / socket fd 所有权与 `Drop` 关闭时机 |
| **9.4** | 理解阻塞 I/O 底层 syscall 成本 |
