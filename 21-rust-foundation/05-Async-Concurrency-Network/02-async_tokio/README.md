# 02 · async_tokio — Async Rust 学习区

> 所属：[05-Async-Concurrency-Network](../README.md) · **02/03** · 前置 [`01-atomic/`](../01-atomic/README-学习区.md) · 下一步 [`03-rust_network_programming/`](../03-rust_network_programming/README.md) stage07

> **小节 `X.Y-slug.md`** = 该节**完整精读**（概念、表格、代码片段、与书的对应）  
> **`本章学习笔记.md`** = 章内索引表（链到各节 + demo 目录）  
> **`X.Y-slug/`** = 每节**至少一个** `*-demo.rs`（与书 § 对齐）  
> 规范：[../01-atomic/小节笔记与Demo规范.md](../01-atomic/小节笔记与Demo规范.md) · 对照：[章节与小节对照表.md](./章节与小节对照表.md)

## 目录约定

```
ch11_async_testing_debugging/
├── 本章学习笔记.md              ← 索引（8 节）
└── 11.3-testing-for-deadlocks/
    ├── 11.3-testing-for-deadlocks.md  ← 精读正文
    └── code/
        └── 11.3-testing-for-deadlocks-timeout-demo.rs
```

## 如何运行 Demo

**仅需 std**（第 10 章部分、占位小节）：

```bash
cd 05-Async-Concurrency-Network/02-async_tokio/ch10_dependency_free_async_server/10.2-building-std-async-runtime
rustc 10.2-building-std-async-runtime-noop-waker-demo.rs
```

**需要 Tokio**（多数章节）：在仓库根 `01-atomic/` 工程已含 Tokio 时，可将 demo 拷入 `examples/`，或在本机：

```bash
# 单文件（需本机已 cargo init 且 Cargo.toml 含 tokio）
rustc --edition 2021 --crate-type bin your-demo.rs  # 通常仍用 cargo 更省事
```

推荐：为常用 demo 在 `atomic/Cargo.toml` 增加 `[[example]]` 指向 `../02-async_tokio/...`（可按需自行添加）。

## 维护脚本

```bash
python 05-Async-Concurrency-Network/scripts/restructure-02-03-like-ch01.py
python 05-Async-Concurrency-Network/scripts/fix-02-03-links.py
```

## 章节目录

| 章 | 文件夹 | 索引 |
|----|--------|------|
| 1–11 | `ch01_async_intro` … `ch11_async_testing_debugging` | 各章 `本章学习笔记.md` |

完整 § 对照见 [章节与小节对照表.md](./章节与小节对照表.md)。
