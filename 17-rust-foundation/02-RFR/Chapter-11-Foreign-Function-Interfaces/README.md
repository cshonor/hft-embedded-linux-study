# 第 11 章：外部函数接口 (FFI)

> **Rust for Rustaceans** · [全书目录](../RFR-本书目录.md)  
> 与 C/C++ 共享地址空间与调用约定；在 **unsafe 边界之上**封装安全 API。

## 本章结构（与原书对齐）

**4 个主节** · 连同二级子节共 **10 个部分**（2 个带子的主节标题 + 2 + 4 + 1 + Summary）。

| 主节 | 英文 | 子节 / 笔记 |
|------|------|-------------|
| **1** | Crossing Boundaries with extern | [01 符号](./01-symbols.md) · [02 调用约定](./02-calling-conventions.md) |
| **2** | Types Across Language Boundaries | [03 类型匹配](./03-type-matching.md) · [04 分配](./04-allocations.md) · [05 回调](./05-callbacks.md) · [06 安全封装](./06-safety.md) |
| **3** | bindgen and Build Scripts | [07 bindgen 与构建脚本](./07-bindgen-build-scripts.md) |
| **4** | Summary | [08 小结](./08-summary.md) |

## 与 The Book / ER 对照

| 主题 | 本仓库 |
|------|--------|
| unsafe / FFI | [19.1 不安全 Rust](../../00-Book/19-advanced-features/19.1-不安全Rust.md) |
| ER Item 34 | [FFI 边界](../../01-ER/Chapter-06-Beyond-Standard-Rust/Item-34-ffi-boundaries/README.md) |
| bindgen demo | [Item 35 demo](../../01-ER/Chapter-06-Beyond-Standard-Rust/Item-35-bindgen/demo-bindgen/) |
| 布局 | [第 2 章 · repr(C)](../Chapter-02-Types/02-layout.md) |

## 旧版单文件

见 git 中的 `11-外部函数接口-FFI-深度解析.md`。
