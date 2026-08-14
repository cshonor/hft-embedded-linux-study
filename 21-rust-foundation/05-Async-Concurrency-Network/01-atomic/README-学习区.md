# 01 · Rust Atomics and Locks — 章节学习区

> 所属：[05-Async-Concurrency-Network](../README.md) · **01/03** · 推荐 **Nomicon 通读后**开始 · 下一步 [`02-async_tokio/`](../02-async_tokio/README.md)

本目录 **`01-atomic/`** 作为**总区**：与 **`Cargo.toml`**、**`src/`** 平级，下面直接是 **10 个英文命名的 `Chapter-…` 文件夹**。

## 两块内容怎么分

| 位置 | 用途 |
|------|------|
| **`src/mod.rs`** | `study_atomic` 库入口：用 **`#[path = "../Chapter-01-…/mod.rs"]`** 等方式挂接各章模块。 |
| **`Chapter-01`～`Chapter-10`** | **章入口** = **`本章学习笔记.md`**；**节** = `X.Y-slug/X.Y-slug.md` + 子笔记 + `code/` demo（如有） |
| **对照表** | [章节与小节对照表.md](./章节与小节对照表.md) · [全书目录-与实体书一致.md](./全书目录-与实体书一致.md) |

构建：`cargo build -p study_atomic` 或根目录 `cargo run -- atomic`。

**跨章专题笔记**：[互斥锁与锁体系-贯通笔记.md](./互斥锁与锁体系-贯通笔记.md) · [RwLock与读写锁体系-贯通笔记.md](./RwLock与读写锁体系-贯通笔记.md) · [Condvar与条件变量-贯通笔记.md](./Condvar与条件变量-贯通笔记.md) · [无锁编程-贯通笔记.md](./无锁编程-贯通笔记.md) · [Atomics与内存序-贯通笔记.md](./Atomics与内存序-贯通笔记.md)

## 章节目录（文件夹名 = 英文）

| 文件夹名 | 主题（中文） | 结构 |
|----------|----------------|------|
| `Chapter-01-Rust-Concurrency-Basics` | Rust 并发基础 | `1.Y-slug/` + `code/` |
| `Chapter-02-Atomics` | 原子操作 | 同左 |
| `Chapter-03-Memory-Ordering` | 内存排序 | 同左 · [贯通笔记](./Atomics与内存序-贯通笔记.md) |
| `Chapter-04-Spin-Locks` | 构建自旋锁 | 同左 · RA 样板见 `4.1` code |
| `Chapter-05-Channels` | 构建通道 | 同左 · demo 在 `5.1`/`5.2` code |
| `Chapter-06-Custom-Arc` | 构建自定义 Arc | 同左 |
| `Chapter-07-Processors` | 理解处理器 | 同左 · demo 在 `7.2`/`7.3` code |
| `Chapter-08-OS-Primitives` | OS 同步原语 | 同左 · [Condvar贯通](./Condvar与条件变量-贯通笔记.md) |
| `Chapter-09-Custom-Locks` | 手写 Mutex/Condvar/RwLock | 同左 · 各锁贯通笔记 |
| `Chapter-10-Advanced-Concurrent-Data-Structures` | 进阶模式与全书收尾 | 同左 · [无锁贯通](./无锁编程-贯通笔记.md) |
