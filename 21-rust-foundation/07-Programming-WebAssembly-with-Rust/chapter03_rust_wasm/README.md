# 第 3 章 · 使用 Rust 涉足 WebAssembly (Wading into WebAssembly with Rust)

> 所属：[07 Wasm+Rust](../README.md) · 目录：[Wasm-本书目录.md](../Wasm-本书目录.md)  
> **Layer 2**：[layer02 订单簿 Wasm](../layer02_orderbook-query-wasm/README.md) · 前置：[第 2 章 · wast 跳棋](../chapter02_wasm_checkers/README.md)

**章导读**：在 Ch1～2 手写 wast 之后，用 **Rust + Cargo + `wasm32-unknown-unknown`** 构建 Wasm 模块；以 **Rusty Checkers** 展示 struct/enum/迭代器、**领域/Wasm 分层**、全局状态与 **`extern "C"`** 宿主互操作。

---

<!-- AUTO:SECTION-INDEX -->

| 节 | 主题 | 笔记 | 状态 |
|:---:|------|------|:----:|
| **3.1** | Introducing Rust | [3.1-introducing-rust.md](./3.1-introducing-rust.md) | ✅ |
| **3.2** | Installing Rust | [3.2-installing-rust.md](./3.2-installing-rust.md) | ✅ |
| **3.3** | Building Hello WebAssembly in Rust | [3.3-hello-wasm-rust.md](./3.3-hello-wasm-rust.md) | ✅ |
| **3.4** | Creating Rusty Checkers | [3.4-rusty-checkers.md](./3.4-rusty-checkers.md) | ✅ |
| **3.5** | Coding the Rusty Checkers WebAssembly Interface | [3.5-rusty-checkers-ffi.md](./3.5-rusty-checkers-ffi.md) | ✅ |
| **3.6** | Playing Rusty Checkers in JavaScript | [3.6-rusty-checkers-js.md](./3.6-rusty-checkers-js.md) | ✅ |
| **3.7** | Wrapping Up | [3.7-wrap-up.md](./3.7-wrap-up.md) | ✅ |

<!-- /AUTO:SECTION-INDEX -->

> 各节 `.md` 为**笔记本体**；README 仅为索引。

---

## 本章速览

| 块 | 要点 |
|----|------|
| **工具链** | `rustup target add wasm32-unknown-unknown` · `cdylib` · `no_mangle` |
| **设计** | 领域引擎 vs Wasm 薄边界 · `Into<i32>` · RwLock 全局状态 |
| **宿主** | JS `fetch` wasm · import 挂在 **`env`** |
| **对比 Ch2** | 体积↑ · 可读性/可测性↑ |
