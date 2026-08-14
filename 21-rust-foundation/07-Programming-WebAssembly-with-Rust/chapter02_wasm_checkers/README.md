# 第 2 章 · 构建 WebAssembly 跳棋 (Building WebAssembly Checkers)

> 所属：[07 Wasm+Rust](../README.md) · 目录：[Wasm-本书目录.md](../Wasm-本书目录.md)  
> **Layer 1→2**：[layer02 订单簿位布局](../layer02_orderbook-query-wasm/README.md) · 前置：[第 1 章](../chapter01_wasm_fundamentals/README.md)

**章导读**：用**纯 wast/wat** 从零构建功能性跳棋模块；在 Wasm **数据结构限制**下实现状态管理、规则、移动与宿主通知。编译后约 **853 字节**、**256 字节**线性内存。

---

<!-- AUTO:SECTION-INDEX -->

| 节 | 主题 | 笔记 | 状态 |
|:---:|------|------|:----:|
| **2.1** | Playing Checkers, the Board Game | [2.1-checkers-board-game.md](./2.1-checkers-board-game.md) | ✅ |
| **2.2** | Coping with Data Structure Constraints | [2.2-data-structure-constraints.md](./2.2-data-structure-constraints.md) | ✅ |
| **2.3** | Implementing Game Rules | [2.3-game-rules.md](./2.3-game-rules.md) | ✅ |
| **2.4** | Moving Players | [2.4-moving-players.md](./2.4-moving-players.md) | ✅ |
| **2.5** | Testing Wasm Checkers | [2.5-testing-wasm-checkers.md](./2.5-testing-wasm-checkers.md) | ✅ |
| **2.6** | Wrapping Up | [2.6-wrap-up.md](./2.6-wrap-up.md) | ✅ |

<!-- /AUTO:SECTION-INDEX -->

> 各节 `.md` 为**笔记本体**；README 仅为索引。

---

## 本章速览

| 块 | 要点 |
|----|------|
| **内存布局** | `offset = (x + y * 8) * 4` · 256B 棋盘 |
| **位标志** | 0 空 · 1 黑 · 2 白 · 4 加冕（例：加冕黑 = 5） |
| **API** | export `$move`/`$initBoard` · 私有 `$setPiece` · import `notify_*` |
| **体积** | ~853B `.wasm` |
