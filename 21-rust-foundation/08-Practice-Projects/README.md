# 08 · 实战练手项目

> 所属：[Rust_study](../README.md) · 笔记主线：`03-DeepRustStdLib` Ch9～14 · `05-Async` · `06` Lox

**定位**：三个**独立可演进**的 Rust 工程，与「看书笔记」分离；代码在本目录，学习记录仍链回各专题笔记。

---

## 项目一览

| # | 目录 | 一句话 | 主要技术 | 学习挂钩 |
|:---:|------|--------|----------|----------|
| **1** | [01-cdn-edge](./01-cdn-edge/README.md) | 轻量 HTTP 边缘缓存节点 | tokio · axum · 内存缓存 | [Ch13 I/O](../03-DeepRustStdLib/chapter13_io/README.md) · [Ch14 async](../03-DeepRustStdLib/chapter14_async/README.md) |
| **2** | [02-midfreq-trading-bot](./02-midfreq-trading-bot/README.md) | 中低频量化框架（秒～分钟级） | tokio · 策略 trait · 风控 · SQLite（规划） | [Ch11 并发](../03-DeepRustStdLib/chapter11_concurrency/README.md) · [05-async](../05-Async-Concurrency-Network/02-async_tokio/README.md) |
| **3** | [03-rlox](./03-rlox/README.md) | Crafting Interpreters 风格 Lox 解释器 | scanner → parser → 树遍历（jlox 路线） | [06 CI · jlox](../06_Compilers-and-LLVM-Learning/01_Crafting-Interpreters/part02_jlox/README.md) |

**建议实现顺序**：`03-rlox` → `01-cdn-edge` → `02-midfreq-trading-bot`

---

## Workspace 构建

**环境**：`cdn-edge` / `midfreq-trading-bot` 依赖 tokio/axum，Windows 需安装 **Visual Studio Build Tools（C++）** 或完整 MSVC 工具链（`link.exe`）。`rlox` 无第三方依赖，可先单独 `cargo check -p rlox`。

```bash
# 在仓库根目录
cargo build --manifest-path 08-Practice-Projects/Cargo.toml

# 单项目
cargo run -p cdn-edge -- --port 8080 --origin ./origin
cargo run -p rlox
cargo run -p midfreq-trading-bot
```

---

## 与 GitHub 对外仓库

本目录代码可 **整目录复制** 或 **subtree split** 为三个独立 GitHub 仓库；README 中已预留「对外描述」段落，便于粘贴到 Profile。
