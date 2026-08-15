# Rust_study

Rust 学习笔记仓库：**以看书、整理笔记为主**；The Book 作语法底座，三书递进通读。

---

## 学习路线（当前采用）

### 语法底座（并行或先行）

| 资源 | 入口 | 说明 |
|------|------|------|
| **The Book** | [`00-Book/Book-本书目录.md`](00-Book/Book-本书目录.md) | 官方教程；建议先过 **1～10 章**（所有权、类型、错误、trait）再通读下面三书 |

### 纯阅读主线（固定顺序）

```text
① RFR  →  ② ER  →  ③ Deep Rust StdLib  →  ④ Nomicon
```

| 顺序 | 书 / 专题 | 本地索引 | 一句话 |
|:---:|-----|----------|--------|
| **1** | *Rust for Rustaceans* | [`02-RFR/RFR-本书目录.md`](02-RFR/RFR-本书目录.md) | 内存、类型、trait、异步原理等**内功** |
| **2** | *Effective Rust* | [`01-ER/ER-本书目录.md`](01-ER/ER-本书目录.md) | 35 条**工程习惯**（Item 笔记 + 可选 demo） |
| **3** | **Deep Rust StdLib** | [`03-DeepRustStdLib/README.md`](03-DeepRustStdLib/README.md) | **标准库**设计与模块语义（进阶） |
| **4** | *The Rustonomicon* | [`04-Rust-Nomicon/README.md`](04-Rust-Nomicon/README.md) | **unsafe**、布局、型变、FFI |

详细说明、节奏与易混点验证建议 → [`docs/纯阅读路线.md`](docs/纯阅读路线.md)

**节奏**：以笔记为主；仅在 `dyn Trait`、`Fn*`、静/动态分发等易混处写几行代码验证，**不必**做整套练习项目。

### Nomicon 之后：并发 · 异步 · 网络 → Wasm → LLVM

```text
⑤ atomic → async_tokio → rust_network
⑦ WebAssembly（Rust → Wasm；可与 ⑤ 后期并行）
⑥a C++ 前置：姊妹仓 cpp-learning-notes 01～06（开 LLVM 前必修，见 06 README）
⑥b Compilers / Learn LLVM 17（Rust 导出 IR 对照；与 ⑦ 栈式 VM / WAT 并排）
```

| 顺序 | 专题 | 入口 |
|:---:|------|------|
| **5** | 并发 / 异步 / 网络 | [`05-Async-Concurrency-Network/README.md`](05-Async-Concurrency-Network/README.md) |
| **7** | **WebAssembly** | [`07-Programming-WebAssembly-with-Rust/README.md`](07-Programming-WebAssembly-with-Rust/README.md) · [知识链](07-Programming-WebAssembly-with-Rust/学习路径与知识链.md) |
| **6a** | **C++ 前置**（外部） | [cpp-learning-notes](https://github.com/cshonor/cpp-learning-notes) **`01`～`06`** → 再开 LLVM |
| **6b** | 编译器 / LLVM | [`06_Compilers-and-LLVM-Learning/README.md`](06_Compilers-and-LLVM-Learning/README.md) |

---

## 目录结构

| 路径 | 内容 |
|------|------|
| [`00-Book/`](00-Book/Book-本书目录.md) | The Book 笔记 + 按章 demo（`00-Book/01-*`～`19-*`） |
| [`01-ER/`](01-ER/ER-本书目录.md) | Effective Rust；每 Item 一个目录（`README.md` + 同级 `*.md` 笔记 + 可选 `demo/`），见 [`01-ER/目录结构.md`](01-ER/目录结构.md) |
| [`02-RFR/`](02-RFR/RFR-本书目录.md) | Rust for Rustaceans 深度笔记（`Chapter-01-*`～`13-*`） |
| [`03-DeepRustStdLib/`](03-DeepRustStdLib/README.md) | **标准库进阶**（ER 之后、Nomicon 之前） |
| [`04-Rust-Nomicon/`](04-Rust-Nomicon/README.md) | Nomicon 笔记；`Stable/` 与 `Nightly/` 对照 |
| [`05-Async-Concurrency-Network/`](05-Async-Concurrency-Network/README.md) | **01-atomic** · **02-async_tokio** · **03-rust_network_programming**（按序读） |
| [`06_Compilers-and-LLVM-Learning/`](06_Compilers-and-LLVM-Learning/README.md) | 编译器四书 · 01～04（含 [Learn LLVM 17](./06_Compilers-and-LLVM-Learning/04_Learn-LLVM-17/README.md) + `llvm_insight_lab`） |
| [`07-Programming-WebAssembly-with-Rust/`](07-Programming-WebAssembly-with-Rust/README.md) | **Kevin Hoffman** — Rust → **Wasm**（`07_WebAssembly`）；[三层练手](07-Programming-WebAssembly-with-Rust/三层学习架构.md) · 衔接 **06** IR · **HFT** |
| [`08-Practice-Projects/`](08-Practice-Projects/README.md) | **实战三项目**：CDN 边缘缓存 · 中低频交易 bot · **rlox** 解释器 |
| [`docs/`](docs/) | 学习路线、编辑器配置等 |

**demo 约定**（The Book / 部分 ER Item）：一个 `*.md` 可对应一个独立 Cargo 工程，例如 `3.3-函数.md` ↔ `3.3-functions-demo/`。

---

## 快速开始

### ER Item demo（示例）

```bash
cd 01-ER
cargo run -p item-02-callbacks-generics
cargo test -p item-03-option-result   # 其他 Item 见各目录 README
```

### The Book demo（示例）

```bash
cd 00-Book/03-common-concepts/3.3-functions-demo
cargo run
```

猜数字：`00-Book/02-guessing-game` · 第 19 章高级特性 → [`00-Book/19-advanced-features/19-章节导读.md`](00-Book/19-advanced-features/19-章节导读.md)

### 并发 / 网络 demo（示例）

```bash
cargo build --manifest-path 05-Async-Concurrency-Network/01-atomic/Cargo.toml
cargo run --manifest-path 05-Async-Concurrency-Network/03-rust_network_programming/stage03_std_tcp_udp/Cargo.toml --bin demo_3_1_tcp_echo_server
```

### 实战项目（08）

```bash
cargo build --manifest-path 08-Practice-Projects/Cargo.toml
cargo run -p cdn-edge -- --port 8080 --origin 08-Practice-Projects/01-cdn-edge/origin
cargo run -p midfreq-trading-bot
cargo run -p rlox
```

---

## Rust 工具链：Stable / Nightly / Edition

与具体 demo 无关；读 **Nomicon** 时常需区分 Stable / Nightly 语境。

**核心**：源码不变，**选哪个 `rustc` 编译，就遵循哪套规则**。

| Channel | 含义 |
|---------|------|
| **Stable** | 发布日冻结规则；禁止 `#![feature(...)]` |
| **Nightly** | 跟 `main` 最新规则；Nomicon 试验写法 |

```bash
cargo +stable build
cargo +nightly build
```

**Edition**（2018 / 2021 / 2024）管语法拼写；**Channel** 管可用 API、feature、检查逻辑——二者独立。

| 文档 | 建议编译器 |
|------|------------|
| Nomicon（可上线 unsafe） | `+stable` → [`04-Rust-Nomicon/Stable/`](04-Rust-Nomicon/Stable/notes.md) |
| Nomicon（前沿 / feature） | `+nightly` → [`04-Rust-Nomicon/Nightly/`](04-Rust-Nomicon/Nightly/notes.md) |

详解：[`00-Book/appendix/G.7-Nightly-Rust与发布渠道.md`](00-Book/appendix/G.7-Nightly-Rust与发布渠道.md) · ER CI/MSRV → [`01-ER/ER-demos/WORKSPACE.md`](01-ER/ER-demos/WORKSPACE.md)

---

## 说明

- 章节目录英文命名，避免 Windows 终端乱码。
- `target/` 已在 `.gitignore` 中忽略。
- **VSCode + rust-analyzer**：[`docs/rust-analyzer-VSCode配置.md`](docs/rust-analyzer-VSCode配置.md) · 工作区 [`.vscode/settings.json`](.vscode/settings.json)

---

## 姊妹仓库

| 仓库 | 语言 | 说明 |
|------|------|------|
| [cpp-learning-notes](https://github.com/cshonor/cpp-learning-notes) | C++ | **`01`～`06` 为 Learn LLVM 前置（必修）**；`07`～`09` 与 Rust **05 / 06** 并行；详见 [`06/README`](06_Compilers-and-LLVM-Learning/README.md) |
