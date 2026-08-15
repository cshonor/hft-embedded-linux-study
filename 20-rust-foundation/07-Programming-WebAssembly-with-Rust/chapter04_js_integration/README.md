# 第 4 章 · 将 WebAssembly 与 JavaScript 集成 (Integrating WebAssembly with JavaScript)

> 所属：[07 Wasm+Rust](../README.md) · 目录：[Wasm-本书目录.md](../Wasm-本书目录.md)  
> **Layer 2**：[layer02 订单簿 · bindgen + serde](../layer02_orderbook-query-wasm/README.md) · 前置：[第 3 章 · 手写 FFI](../chapter03_rust_wasm/README.md)

**章导读**：引入 **`wasm-bindgen`** 与 **npm/webpack** 脚手架；以 **Rogue + Rot.js** 演示 JS/Rust 职责分离与双向 class 绑定；用 **`serde` / `js_sys` / `web_sys`** 完成 JSON 与 Web API 集成。

---

<!-- AUTO:SECTION-INDEX -->

| 节 | 主题 | 笔记 | 状态 |
|:---:|------|------|:----:|
| **4.1** | Creating a Better Hello, World | [4.1-better-hello-world.md](./4.1-better-hello-world.md) | ✅ |
| **4.2** | Building the Rogue WebAssembly Game | [4.2-rogue-wasm-game.md](./4.2-rogue-wasm-game.md) | ✅ |
| **4.3** | Experimenting Further | [4.3-experimenting-further.md](./4.3-experimenting-further.md) | ✅ |
| **4.4** | Wrapping Up | [4.4-wrap-up.md](./4.4-wrap-up.md) | ✅ |

<!-- /AUTO:SECTION-INDEX -->

> 各节 `.md` 为**笔记本体**；README 仅为索引。

---

## 本章速览

| 块 | 要点 |
|----|------|
| **wasm-bindgen** | 宏 + CLI · String/类 · 自动 linear memory 胶水 |
| **Rogue** | Rot.js (JS) + Engine (Rust) · import Display · export Engine |
| **生态** | serde JSON · js_sys · web_sys · webpack |
