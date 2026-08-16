# 第 1 章 · WebAssembly 基础 (WebAssembly Fundamentals)

> 所属：[07 Wasm+Rust](../README.md) · 目录：[Wasm-本书目录.md](../Wasm-本书目录.md) · **Layer 1**：[layer01_rust-llvm-to-wasm](../layer01_rust-llvm-to-wasm/README.md)

**章导读**：深入 Wasm **底层核心概念与架构机制**，并用 **wabt / WAT** 手写、编译首个模块——为后续 Rust 编译到 Wasm 建立「编译器在背后做了什么」的直觉。

---

<!-- AUTO:SECTION-INDEX -->

| 节 | 主题 | 笔记 | 状态 |
|:---:|------|------|:----:|
| **1.1** | Introducing WebAssembly | [1.1-introducing-wasm.md](./1.1-introducing-wasm.md) | ✅ |
| **1.2** | Understanding WebAssembly Architecture | [1.2-wasm-architecture.md](./1.2-wasm-architecture.md) | ✅ |
| **1.3** | Building a WebAssembly Application | [1.3-building-wasm-app.md](./1.3-building-wasm-app.md) | ✅ |
| **1.4** | Wrapping Up | [1.4-wrap-up.md](./1.4-wrap-up.md) | ✅ |

<!-- /AUTO:SECTION-INDEX -->

> 各节 `.md` 为**笔记本体**；README 仅为索引。

---

## 本章速览

| 块 | 内容 |
|----|------|
| **是什么 / 不是什么** | 栈式 VM 二进制格式 · 编译目标 · 宿主沙盒 · 非转译 · 非取代 JS · 非独立语言 |
| **架构** | 栈式机器 · 四种数值类型 · 安全控制流 · 线性内存（无 GC） |
| **动手** | wabt · WAT · 往返编译 · 反汇编与安全启示 |
