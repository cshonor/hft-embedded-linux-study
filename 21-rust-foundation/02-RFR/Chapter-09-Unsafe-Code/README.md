# 第 9 章：不安全代码 (Unsafe Code)

> **Rust for Rustaceans** · [全书目录](../RFR-本书目录.md)  
> `unsafe` = 编译器无法自动证明不变量时，**开发者显式承担证明责任** — 不是关掉借用检查。

## 本章结构（与原书对齐）

**5 个主节** · 连同二级子节共 **16 个部分**（3 个带子的主节标题 + 1 + 3 + 5 + 3 + Summary）。

| 主节 | 英文 | 子节 / 笔记 |
|------|------|-------------|
| **1** | The unsafe Keyword | [01 unsafe 关键字](./01-unsafe-keyword.md)（双重角色 · 五类超能力 · 借用仍生效） |
| **2** | Great Power | [02 裸指针](./02-raw-pointers.md)（`*const`/`*mut` · `NonNull` · niche） · [03 调用 unsafe 函数](./03-calling-unsafe-functions.md)（FFI · `_unchecked` · `MaybeUninit`） · [04 unsafe trait](./04-unsafe-traits.md)（Send/Sync · Unpin 区分） |
| **3** | Great Responsibility | [05](./05-what-can-go-wrong.md)（）· [06](./06-validity.md)（）· [07](./07-panics.md)（）· [08](./08-casting.md)（）· [09 Drop 检查](./09-drop-check.md)（dropck） |
| **4** | Coping with Fear | [10 管理边界](./10-manage-boundaries.md)（信任域）· [11 文档](./11-documentation.md)（`// SAFETY:`）· [12 验证工作](./12-check-your-work.md)（Miri） |
| **5** | Summary | [13 小结](./13-summary.md) |

## 与 The Book / ER / Nomicon 对照

| 主题 | 本仓库 |
|------|--------|
| unsafe 入门 | [19.1 不安全 Rust](../../00-Book/19-advanced-features/19.1-不安全Rust.md) |
| 避免 unsafe | [ER Item 16](../../01-ER/Chapter-03-Concepts/Item-16-avoid-unsafe/README.md) |
| Miri | [Item 16 demo](../../01-ER/Chapter-03-Concepts/Item-16-avoid-unsafe/demo/) |
| Nomicon | [04-Rust-Nomicon](../../04-Rust-Nomicon/README.md) |
| 布局 | [第 2 章 · Layout](../Chapter-02-Types/02-layout.md) |

## 旧版单文件

见 git 中的 `9-不安全代码-Unsafe-Code-深度解析.md`。
