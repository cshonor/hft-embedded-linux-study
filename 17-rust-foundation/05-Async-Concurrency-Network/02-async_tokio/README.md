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

## 如何运行 Demo（2026-09-12 修订）

> **旧写法已废弃**。此前本目录没有 Cargo 工程，README 教人直接 `rustc X.Y-xxx-demo.rs`，
> 但文件名含 `.`（如 `1.1-what-is-async-join-demo.rs`）会被 rustc 以非法 crate 名拒绝，
> 且多数 demo 依赖 tokio —— **结果就是 84 个 demo 没有一个个跑起来过**。

现在本目录已有 `Cargo.toml`，84 个 demo 全部注册为 `[[bin]]`：

```bash
cd 05-Async-Concurrency-Network/02-async_tokio

cargo check                                  # 全量检查：84 bin / 0 error / 0 warning
cargo run --bin c1_1_what_is_async_join_demo # 跑单个
cargo build                                  # 全部构建
```

### bin 命名规则

原文件**不改名**，改用 `[[bin]]` 显式给出合法 name（非字母数字 → `_`，数字开头补 `c`）：

| 原文件 | bin 名 |
|---|---|
| `ch01_async_intro/1.1-what-is-async/code/1.1-what-is-async-join-demo.rs` | `c1_1_what_is_async_join_demo` |
| `ch07_tokio_graceful_shutdown/7.1-building-a-runtime/code/7.1-building-a-runtime-demo.rs` | `c7_1_building_a_runtime_demo` |

不知道名字就列一下：

```bash
cargo metadata --no-deps --format-version 1 | python -c "import json,sys;[print(t['name']) for t in json.load(sys.stdin)['packages'][0]['targets']]"
```

### 依赖

| crate | 用途 |
|---|---|
| `tokio`（full） | 41 个 demo 的运行时 |
| `tokio-util`（`rt`） | ch07 `LocalPoolHandle` |
| `mio` | ch04/4.7–4.8 轮询 socket |
| `reqwest` | ch01/1.6 HTTP 性能对比 |
| `futures-lite` | ch03/3.6 `join!` 宏的 `block_on` |
| `flume` | ch03 自定义任务队列通道 |

### 顺带修掉的 3 个真实编译错误

代码从未编译过，所以藏着真 bug：

| 文件 | 原错误 | 修法 |
|---|---|---|
| `11.5-testing-channel-capacity-demo.rs` | `JoinHandle` 被 `timeout` 移走后又 `await`；且对 `()` 调 `.unwrap()` | `&mut` 借用 + 去掉多余 `.unwrap()` |
| `2.6-sharing-data-try-lock-demo.rs` | `MutexGuard` 借用到块尾，与 `self.done = true` 冲突（E0502） | 加锁自增圈进独立作用域 |
| `6.5-event-bus-broadcasting-demo.rs` | `Event::Temp(i16)` 负载从未读取 | 让 demo 真正读出并打印负载 |

另有 29 个文件顶部的 `#![crate_name = "..."]`（当年绕开文件名问题的土办法）已移除——
它与 `[[bin]]` 的 `--crate-name` 冲突。

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
