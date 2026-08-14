# 10 · Beneath std · 本章定位

← [本章目录](./README.md) · [全书笔记](../notes.md) · 上一章：[09 FFI](../09_FFI/README.md)

---

官方标题 **Beneath std**。探讨 **`#![no_std]`** 环境下须手动接管的底层机制——OS 内核、嵌入式等场景的关键一章。

| 对照 | 路径 |
|------|------|
| ER Item 33 | [no-std](../../01-ER/Chapter-06-Beyond-Standard-Rust/Item-33-no-std/README.md) |
| RFR no_std | [Ch12 Without std](../../02-RFR/Chapter-12-Rust-Without-Standard-Library/README.md) |
| panic_handler | [09_FFI](../09_FFI/07-unwind.md) |

**读完应能回答**：为何 `libc` 要关 default-features、`no_main` 做什么、`#[panic_handler]` 签名与唯一性。

---

## 小节路线图

```text
01  libc default-features = false
  ↓
02  no_main / _start / lang items
  ↓
03  panic_handler（全局唯一）
```

| 节 | 主题 | 阅读 |
|:--:|------|------|
| — | 本章定位 | 本页 |
| 1 | 引入 libc 依赖 | [01-libc.md](./01-libc.md) |
| 2 | 无 std 可执行程序 | [02-no-main.md](./02-no-main.md) |
| 3 | 自定义 panic 处理 | [03-panic-handler.md](./03-panic-handler.md) |
| — | no_main 裸机模板 | [templates/no_main_linux.md](./templates/no_main_linux.md) |
| — | 速记 · 自测 |

---

## 本仓库 demo

| 构建 | 命令 |
|------|------|
| 宿主 std 演示 | `cargo run` |
| 纯 `#![no_std]` 库 | `cargo build --no-default-features` |

`no_std` **库**可在 stable 构建；**裸机可执行文件**见 [02-no-main.md](./02-no-main.md) 模板（通常 nightly）。

---

## 一句话

**no_std 收官章** — 脱离 std 后接管入口、panic、libc/compiler_builtins；stable 可建 no_std 库，裸机 exe 通常需 nightly。

→ 从 [01-libc.md](./01-libc.md) 起读。
