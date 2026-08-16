# 第 6 章 · 在浏览器外部托管模块 (Hosting Modules Outside the Browser)

> 所属：[07 Wasm+Rust](../README.md) · 目录：[Wasm-本书目录.md](../Wasm-本书目录.md)  
> **Part III 开篇** · **Layer 3**：[host_wasmtime](../layer03_quant-ma-strategy/README.md) · 前置：[第 5 章 · Yew](../chapter05_yew/README.md)

**章导读**：用 **Rust 控制台** 当 Wasm **宿主** — 五项职责 · **`wasmi`** 解释 `add` 模块 · 控制台跑 **Ch2/Ch3 跳棋**（ImportResolver / Externals / 读 linear memory 画 ASCII 棋盘）。

---

<!-- AUTO:SECTION-INDEX -->

| 节 | 主题 | 笔记 | 状态 |
|:---:|------|------|:----:|
| **6.1** | How to Be a Good Host | [6.1-good-host.md](./6.1-good-host.md) | ✅ |
| **6.2** | Interpreting WebAssembly Modules with Rust | [6.2-interpreting-wasm-rust.md](./6.2-interpreting-wasm-rust.md) | ✅ |
| **6.3** | Building a Console Host Checkers Player | [6.3-console-checkers.md](./6.3-console-checkers.md) | ✅ |
| **6.4** | Wrapping Up | [6.4-wrap-up.md](./6.4-wrap-up.md) | ✅ |

<!-- /AUTO:SECTION-INDEX -->

> 各节 `.md` 为**笔记本体**；README 仅为索引。

---

## 本章速览

| 块 | 要点 |
|----|------|
| **宿主五职责** | load/validate · exports · imports · execute · isolate |
| **wasmi** | ModuleInstance · call export |
| **控制台跳棋** | Resolver + Externals · memory → ASCII |
| **结论** | **同一 `.wasm` 跨宿主** |
